#include "cover_cluster.h"
#include "base_components/relay.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include "hal/zigbee.h"

// ============================================================================
// Configuration & Constants
// ============================================================================

// Minimum time between relay state changes - protects relay contacts from
// arc damage during locked rotor current (startup), and prevents mechanical
// shock to motor/gears when reversing direction.
#define RELAY_MIN_SWITCH_TIME_MS    200

static zigbee_cover_cluster *      cover_cluster_by_endpoint[10];
static zigbee_cover_cluster_config nv_config_buffer;

// Window covering type attribute - required by ZCL spec but not actively used.
// Value 0 = Rollershade (liftable cover, not tiltable).
static uint8_t window_covering_type = 0;

// ============================================================================
// Position Tracking Helpers
// ============================================================================

static int cover_has_position_tracking(zigbee_cover_cluster *cluster) {
    return cluster->open_time_ms > 0 && cluster->close_time_ms > 0;
}

static uint8_t cover_get_current_position(zigbee_cover_cluster *cluster) {
    if (!cover_has_position_tracking(cluster)) {
        return cluster->cover_position;
    }
    if (cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
        return cluster->cover_position;
    }

    uint32_t elapsed = hal_millis() - cluster->movement_start_ms;
    int32_t  delta;
    int32_t  pos;

    if (cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING) {
        delta = (int32_t)elapsed * 100 / cluster->open_time_ms;
        pos   = (int32_t)cluster->movement_start_position - delta;
    } else {
        delta = (int32_t)elapsed * 100 / cluster->close_time_ms;
        pos   = (int32_t)cluster->movement_start_position + delta;
    }

    if (pos < 0) {
        return 0;
    } else if (pos > 100) {
        return 100;
    } else {
        return (uint8_t)pos;
    }
}

static void cover_update_position(zigbee_cover_cluster *cluster) {
    if (!cover_has_position_tracking(cluster)) {
        return;
    }

    // Prefer exact target when a scheduled stop fires; fall back to time interpolation for manual stops.
    cluster->cover_position = (cluster->target_position != 0xFF)
        ? cluster->target_position
        : cover_get_current_position(cluster);

    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_WINDOW_COVERING,
                                        ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE);
}

// ============================================================================
// Movement Control
// ============================================================================

/**
 * Immediately applies the requested movement state to the relays.
 *
 * This is a low-level function that directly controls the relay hardware
 * without any safety timing checks. Should only be called by cover_request_movement()
 * after verifying timing constraints are satisfied.
 */
void cover_apply_movement(zigbee_cover_cluster *cluster, uint8_t moving) {
    uint8_t  old_moving  = cluster->moving;
    relay_t *open_relay  = cluster->motor_reversal ? cluster->close_relay : cluster->open_relay;
    relay_t *close_relay = cluster->motor_reversal ? cluster->open_relay : cluster->close_relay;

    // Position tracking: update stored position when movement stops
    // (must be called BEFORE cluster->moving is changed so cover_get_current_position works)
    if (moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED &&
        old_moving != ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
        cover_update_position(cluster);
    }

    uint32_t now = hal_millis();
    cluster->last_switch_time = now;
    if (moving == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING) {
        relay_on(open_relay);
        relay_off(close_relay);
        cluster->moving = ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING;
    }else if (moving == ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING) {
        relay_off(open_relay);
        relay_on(close_relay);
        cluster->moving = ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING;
    }else {
        relay_off(open_relay);
        relay_off(close_relay);
        cluster->moving = ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED;
    }

    // Position tracking: capture start position and time when movement begins
    if (old_moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED &&
        moving != ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
        cluster->movement_start_ms       = now;
        cluster->movement_start_position = cluster->cover_position;
    }

    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_WINDOW_COVERING,
                                        ZCL_ATTR_WINDOW_COVERING_MOVING);
}

void cover_schedule_movement(zigbee_cover_cluster *cluster, uint8_t moving, uint32_t delay) {
    cluster->pending_movement     = moving;
    cluster->has_pending_movement = 1;
    hal_tasks_schedule(&cluster->delay_task, delay);
}

/**
 * Requests a movement state change with motor protection timing enforcement.
 *
 * This is the safe, high-level function for all movement requests. It enforces
 * minimum time between relay state changes to protect the motor and relay contacts.
 *
 * If timing constraints aren't met, the movement is scheduled for delayed execution.
 */
void cover_request_movement(zigbee_cover_cluster *cluster, uint8_t moving) {
    // Ignore duplicate requests and cancel any pending delayed operation. This is especially
    // important for some cover switches with stop buttons. Their stop button closes both contacts,
    // so pressing it while moving might initially appear as a repeated/reversal command before the
    // stop command arrives. Canceling pending operations ensures we handle this sequence correctly.
    if (moving == cluster->moving) {
        if (cluster->has_pending_movement) {
            hal_tasks_unschedule(&cluster->delay_task);
        }

        return;
    }

    // Enforce motor protection delay. Minimum time must elapse between relay state changes.
    uint32_t elapsed = hal_millis() - cluster->last_switch_time;
    if (elapsed < RELAY_MIN_SWITCH_TIME_MS) {
        cover_schedule_movement(cluster, moving, RELAY_MIN_SWITCH_TIME_MS - elapsed);
        return;
    }

    // Direct transitions to/from STOP can be applied immediately. Direction reversals require
    // stopping first to avoid damage to the motor and the relays.
    if (moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED ||
        cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
        cover_apply_movement(cluster, moving);
    }else {
        cover_apply_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED);
        cover_schedule_movement(cluster, moving, RELAY_MIN_SWITCH_TIME_MS);
    }
}

static void cover_cancel_target(zigbee_cover_cluster *cluster) {
    hal_tasks_unschedule(&cluster->position_task);
    cluster->target_position = 0xFF;
}

void cover_open(zigbee_cover_cluster *cluster) {
    cover_cancel_target(cluster);
    cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING);
}

void cover_close(zigbee_cover_cluster *cluster) {
    cover_cancel_target(cluster);
    cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING);
}

void cover_stop(zigbee_cover_cluster *cluster) {
    cover_cancel_target(cluster);
    cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED);
}

void cover_delay_handler(void *arg) {
    zigbee_cover_cluster *cluster = (zigbee_cover_cluster *)arg;

    cover_request_movement(cluster, cluster->pending_movement);
}

// Does not call cover_stop() to avoid unnecessary target cancellation.
void cover_position_task_handler(void *arg) {
    zigbee_cover_cluster *cluster = (zigbee_cover_cluster *)arg;

    // Leave target_position set so cover_update_position uses the exact target
    cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED);
    cluster->target_position = 0xFF;
}

void cover_go_to_lift_percentage(zigbee_cover_cluster *cluster, uint8_t target) {
    printf("[cover] go_to_lift target=%d open=%d close=%d pos=%d moving=%d\r\n",
           target, cluster->open_time_ms, cluster->close_time_ms,
           cluster->cover_position, cluster->moving);

    if (!cover_has_position_tracking(cluster)) {
        printf("[cover] go_to_lift: tracking disabled, ignoring\r\n");
        return;
    }

    // Clamp target to valid range
    if (target > 100) {
        target = 100;
    }

    uint8_t current = cover_get_current_position(cluster);

    // Cancel any previous GoToLiftPercentage timer
    hal_tasks_unschedule(&cluster->position_task);

    // If already at target, just stop and return
    if (target == current) {
        printf("[cover] go_to_lift: already at target=%d, stopping\r\n", target);
        cover_cancel_target(cluster);
        cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED);
        return;
    }

    cluster->target_position = target;

    uint8_t dir = (target < current) ? ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING
                                               : ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING;
    uint16_t travel_time = (target < current) ? cluster->open_time_ms : cluster->close_time_ms;
    uint8_t  distance    = (target <
                            current) ? (uint8_t)(current - target) : (uint8_t)(target - current);
    uint32_t run_ms = (uint32_t)distance * travel_time / 100;
    printf("[cover] go_to_lift: %s current=%d -> target=%d run_ms=%d\r\n",
           (dir == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING) ? "OPEN" : "CLOSE",
           current, target, (unsigned)run_ms);
    cover_request_movement(cluster, dir);
    // Ensure at least RELAY_MIN_SWITCH_TIME_MS so the stop isn't deferred by motor protection
    uint32_t schedule_ms = run_ms > RELAY_MIN_SWITCH_TIME_MS ? run_ms : RELAY_MIN_SWITCH_TIME_MS;
    hal_tasks_schedule(&cluster->position_task, schedule_ms);
}

// ============================================================================
// NVM Persistence
// ============================================================================

void cover_cluster_store_attrs_to_nv(zigbee_cover_cluster *cluster) {
    nv_config_buffer.motor_reversal = cluster->motor_reversal;
    nv_config_buffer.open_time_ms   = cluster->open_time_ms;
    nv_config_buffer.close_time_ms  = cluster->close_time_ms;
    nv_config_buffer.last_position  = cluster->cover_position;

    hal_nvm_write(NV_ITEM_COVER_CONFIG(cluster->cover_idx),
                  sizeof(zigbee_cover_cluster_config),
                  (uint8_t *)&nv_config_buffer);
}

void cover_cluster_load_attrs_from_nv(zigbee_cover_cluster *cluster) {
    hal_nvm_status_t st = hal_nvm_read(NV_ITEM_COVER_CONFIG(cluster->cover_idx),
                                       sizeof(zigbee_cover_cluster_config),
                                       (uint8_t *)&nv_config_buffer);

    if (st != HAL_NVM_SUCCESS) {
        printf("[cover] load_nv idx=%d: no record, keeping defaults open=%d close=%d pos=%d\r\n",
               cluster->cover_idx, cluster->open_time_ms, cluster->close_time_ms,
               cluster->cover_position);
        return;
    }

    printf("[cover] load_nv idx=%d: raw reversal=%d open=%d close=%d last_pos=%d\r\n",
           cluster->cover_idx, nv_config_buffer.motor_reversal,
           nv_config_buffer.open_time_ms, nv_config_buffer.close_time_ms,
           nv_config_buffer.last_position);

    cluster->motor_reversal = nv_config_buffer.motor_reversal;
    // Treat 0 in NVM as "unset" and keep the init default - earlier firmware
    // persisted 0 here, which would otherwise permanently disable position tracking.
    if (nv_config_buffer.open_time_ms != 0) {
        cluster->open_time_ms = nv_config_buffer.open_time_ms;
    }
    if (nv_config_buffer.close_time_ms != 0) {
        cluster->close_time_ms = nv_config_buffer.close_time_ms;
    }
    cluster->cover_position = nv_config_buffer.last_position;

    printf("[cover] load_nv idx=%d: applied open=%d close=%d pos=%d tracking=%d\r\n",
           cluster->cover_idx, cluster->open_time_ms, cluster->close_time_ms,
           cluster->cover_position, cover_has_position_tracking(cluster));
}

// ============================================================================
// Attribute & Command Handlers
// ============================================================================

void cover_cluster_on_write_attr(zigbee_cover_cluster *cluster, uint16_t attribute_id) {
    printf("[cover] write_attr ep=%d attr=0x%04x reversal=%d open=%d close=%d\r\n",
           cluster->endpoint, attribute_id, cluster->motor_reversal,
           cluster->open_time_ms, cluster->close_time_ms);
    switch (attribute_id) {
    case ZCL_ATTR_WINDOW_COVERING_MOTOR_REVERSAL:
        cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED);
        cover_cluster_store_attrs_to_nv(cluster);
        break;
    case ZCL_ATTR_WINDOW_COVERING_OPEN_TIME:
    case ZCL_ATTR_WINDOW_COVERING_CLOSE_TIME:
        cover_cluster_store_attrs_to_nv(cluster);
        break;
    }
}

void cover_cluster_callback_attr_write_trampoline(uint8_t endpoint, uint16_t attribute_id) {
    if (cover_cluster_by_endpoint[endpoint] == NULL) {
        return;
    }
    cover_cluster_on_write_attr(cover_cluster_by_endpoint[endpoint], attribute_id);
}

hal_zigbee_cmd_result_t cover_cluster_callback(zigbee_cover_cluster *cluster,
                                               uint8_t command_id,
                                               void *cmd_payload,
                                               uint16_t cmd_payload_len) {
    printf("[cover] cmd cb ep=%d cmd=0x%02x len=%d\r\n",
           cluster->endpoint, command_id, cmd_payload_len);
    switch (command_id) {
    case ZCL_CMD_WINDOW_COVERING_UP_OPEN:
        cover_open(cluster);
        break;
    case ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE:
        cover_close(cluster);
        break;
    case ZCL_CMD_WINDOW_COVERING_STOP:
        cover_stop(cluster);
        break;
    case ZCL_CMD_WINDOW_COVERING_GO_TO_LIFT_PERCENTAGE:
        if (cmd_payload_len < 1) {
            return HAL_ZIGBEE_CMD_SKIPPED;
        }
        cover_go_to_lift_percentage(cluster, ((uint8_t *)cmd_payload)[0]);
        break;
    default:
        printf("Unknown cover command: %d\r\n", command_id);
        return(HAL_ZIGBEE_CMD_SKIPPED);
    }

    return(HAL_ZIGBEE_CMD_PROCESSED);
}

hal_zigbee_cmd_result_t cover_cluster_callback_trampoline(uint8_t endpoint,
                                                          uint16_t cluster_id,
                                                          uint8_t command_id,
                                                          void *cmd_payload,
                                                          uint16_t cmd_payload_len) {
    return(cover_cluster_callback(cover_cluster_by_endpoint[endpoint], command_id,
                                  cmd_payload, cmd_payload_len));
}

// ============================================================================
// Initialization
// ============================================================================

void cover_cluster_init(zigbee_cover_cluster *cluster) {
    // Attributes
    cluster->cover_position = 50;   // unknown default
    cluster->moving         = ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED;
    cluster->motor_reversal = 0;
    cluster->open_time_ms   = 0;
    cluster->close_time_ms  = 0;

    // State
    cluster->last_switch_time        = 0;
    cluster->pending_movement        = 0;
    cluster->has_pending_movement    = 0;
    cluster->movement_start_ms       = 0;
    cluster->movement_start_position = 50;
    cluster->target_position         = 0xFF;

    hal_tasks_init(&cluster->delay_task);
    cluster->delay_task.handler = cover_delay_handler;
    cluster->delay_task.arg     = cluster;

    hal_tasks_init(&cluster->position_task);
    cluster->position_task.handler = cover_position_task_handler;
    cluster->position_task.arg     = cluster;
}

void cover_cluster_add_to_endpoint(zigbee_cover_cluster *cluster, hal_zigbee_endpoint *endpoint) {
    printf("[cover] add_to_endpoint ep=%d idx=%d cluster_count_before=%d\r\n",
           endpoint->endpoint, cluster->cover_idx, endpoint->cluster_count);
    cover_cluster_by_endpoint[endpoint->endpoint] = cluster;
    cluster->endpoint = endpoint->endpoint;
    cover_cluster_init(cluster);
    cover_cluster_load_attrs_from_nv(cluster);

    SETUP_ATTR(0,
               ZCL_ATTR_WINDOW_COVERING_TYPE,
               ZCL_DATA_TYPE_ENUM8,
               ATTR_READONLY,
               window_covering_type);
    SETUP_ATTR(1,
               ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE,
               ZCL_DATA_TYPE_UINT8,
               ATTR_READONLY,
               cluster->cover_position);
    SETUP_ATTR(2,
               ZCL_ATTR_WINDOW_COVERING_MOVING,
               ZCL_DATA_TYPE_ENUM8,
               ATTR_READONLY,
               cluster->moving);
    SETUP_ATTR(3,
               ZCL_ATTR_WINDOW_COVERING_MOTOR_REVERSAL,
               ZCL_DATA_TYPE_BOOLEAN,
               ATTR_WRITABLE,
               cluster->motor_reversal);
    SETUP_ATTR(4,
               ZCL_ATTR_WINDOW_COVERING_OPEN_TIME,
               ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE,
               cluster->open_time_ms);
    SETUP_ATTR(5,
               ZCL_ATTR_WINDOW_COVERING_CLOSE_TIME,
               ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE,
               cluster->close_time_ms);

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_WINDOW_COVERING;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 6;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    = cover_cluster_callback_trampoline;
    endpoint->cluster_count++;
    printf(
        "[cover] add_to_endpoint ep=%d registered cluster=0x%04x is_server=1 cluster_count_after=%d\r\n",
        endpoint->endpoint, ZCL_CLUSTER_WINDOW_COVERING, endpoint->cluster_count);
}
