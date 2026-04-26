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

// Extra duration appended to every move that targets a physical end position (fully open or
// fully closed). Window covers drift over time when operated in a sub-range (e.g. 0–50%):
// small timing errors accumulate until the reported position no longer matches reality.
// Applying an overrun on every end-position arrival forces the motor past the reported end,
// re-aligning the physical cover with the tracker on each use. The position is clamped to
// 0% / 100% as soon as the tracker reaches the end, so the reported position is always
// correct during the overrun period.
#define OVERRUN_DURATION_MS    3000

// Safety timeout used in manual mode: the maximum time a relay can remain
// energized when position tracking is disabled. Long enough to accommodate even
// very slow covers, and ensures the relay is eventually de-energized if the user
// walks away without stopping the motor.
#define MANUAL_MODE_TIMEOUT_MS    300000

static zigbee_cover_cluster *      cover_cluster_by_endpoint[10];
static zigbee_cover_cluster_config nv_config_buffer;

// Window covering type attribute - required by ZCL spec but not actively used.
// Value 0 = Rollershade (liftable cover, not tiltable).
static uint8_t window_covering_type = 0;

// ============================================================================
// Position Tracking Engine
// ============================================================================

#define MAX_MOTOR_POSITION    10000

/**
 * Returns the travel time in milliseconds for the given direction. If close_time is not
 * configured, the open_time is used for both directions. This allows a single open_time value to
 * cover both directions for symmetric covers.
 */
static uint16_t cover_travel_time(zigbee_cover_cluster *cluster, uint8_t direction) {
    uint16_t travel_time = cluster->open_time;

    if (direction == ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING && cluster->close_time > 0) {
        travel_time = cluster->close_time;
    }

    return travel_time * 100; // Convert from tenth of seconds to milliseconds
}

/**
 * Converts the internal motor position (0–10000 basis points) to the ZCL
 * cover position percentage (0–100), applying open/closed deadzones.
 *
 * Positions at or above the open deadzone clamp to 100.
 * Positions at or below the closed deadzone clamp to 0.
 * Values in between are linearly mapped across the effective range.
 */
uint8_t motor_to_cover_position(zigbee_cover_cluster *cluster) {
    uint32_t open_deadzone_bp   = (uint32_t)cluster->open_deadzone * 100;
    uint32_t closed_deadzone_bp = (uint32_t)cluster->closed_deadzone * 100;

    if (cluster->motor_position >= (10000 - open_deadzone_bp)) {
        return 100;
    }

    if (cluster->motor_position <= closed_deadzone_bp) {
        return 0;
    }

    uint32_t effective_bp = 10000 - open_deadzone_bp - closed_deadzone_bp;
    uint32_t travel_bp    = cluster->motor_position - closed_deadzone_bp;
    return (travel_bp * 100) / effective_bp;
}

/**
 * Converts a ZCL cover position percentage (0–100) to the internal motor
 * position in basis points (0–10000), applying open/closed deadzones.
 *
 * 0 maps to 0, 100 maps to 10000. Intermediate values are linearly mapped
 * across the effective range between the deadzones.
 */
uint16_t cover_to_motor_position(zigbee_cover_cluster *cluster, uint8_t cover_pos) {
    if (cover_pos == 0) return 0;

    if (cover_pos >= 100) return 10000;

    uint32_t open_deadzone_bp   = (uint32_t)cluster->open_deadzone * 100;
    uint32_t closed_deadzone_bp = (uint32_t)cluster->closed_deadzone * 100;
    uint32_t effective_bp       = 10000 - open_deadzone_bp - closed_deadzone_bp;
    return closed_deadzone_bp + (cover_pos * effective_bp) / 100;
}

void cover_update_position(zigbee_cover_cluster *cluster) {
    if (cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
        return;
    }

    uint32_t elapsed_ms     = hal_millis() - cluster->movement_start_time;
    uint32_t travel_time_ms = cover_travel_time(cluster, cluster->moving);
    uint32_t motor_position_delta;
    if (travel_time_ms == 0) {
        motor_position_delta = MAX_MOTOR_POSITION;
    } else {
        motor_position_delta = (elapsed_ms * MAX_MOTOR_POSITION) / travel_time_ms;
    }

    if (cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING) {
        uint32_t new_motor_position = cluster->start_motor_position + motor_position_delta;
        if (new_motor_position > MAX_MOTOR_POSITION) {
            new_motor_position = MAX_MOTOR_POSITION;
        }
        cluster->motor_position = (uint16_t)new_motor_position;
    } else {
        if (motor_position_delta > cluster->start_motor_position) {
            cluster->motor_position = 0;
        } else {
            cluster->motor_position = cluster->start_motor_position -
                                      (uint16_t)motor_position_delta;
        }
    }

    uint8_t new_cover_pos = motor_to_cover_position(cluster);
    if (new_cover_pos != cluster->position) {
        cluster->position = new_cover_pos;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_WINDOW_COVERING,
                                            ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE);
    }
}

void cover_cancel_movement_tasks(zigbee_cover_cluster *cluster) {
    hal_tasks_unschedule(&cluster->stop_task);
    hal_tasks_unschedule(&cluster->update_task);
}

void cover_schedule_next_update(zigbee_cover_cluster *cluster) {
    // Update position every 1% of travel, but not more often than every 100ms.
    uint32_t update_interval_ms = cover_travel_time(cluster, cluster->moving) / 100;

    if (update_interval_ms < 100) {
        update_interval_ms = 100;
    }

    hal_tasks_schedule(&cluster->update_task, update_interval_ms);
}

void cover_update_handler(void *arg) {
    zigbee_cover_cluster *cluster = (zigbee_cover_cluster *)arg;

    cover_update_position(cluster);

    if (cluster->moving != ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
        cover_schedule_next_update(cluster);
    }
}

// ============================================================================
// Movement Control
// ============================================================================

/**
 * Manual mode is active when both open_time and close_time are zero. In this
 * mode position tracking is disabled: the position attribute snaps immediately to
 * 0% or 100% on move commands, GoToLiftPercentage commands are ignored, and a
 * fixed MANUAL_MODE_TIMEOUT_MS safety timeout replaces the travel-time-based stop.
 */
static bool cover_is_manual_mode(const zigbee_cover_cluster *cluster) {
    return cluster->open_time == 0 && cluster->close_time == 0;
}

/**
 * Immediately executes an action: sets the relays and schedules the stop_task.
 * COVER_ACTION_STOP stops the motor.  Returns early if action equals the
 * current motor position (already at target).
 *
 * In manual mode (both open_time and close_time are zero), position tracking
 * is disabled: the position attribute snaps immediately to 0%/100%, and a fixed
 * MANUAL_MODE_TIMEOUT_MS safety timeout is used instead of the travel-time-based stop.
 *
 * This is a low-level function that directly controls the relay hardware
 * without any safety timing checks. Should only be called by cover_dispatch_action()
 * or timer callbacks that already know the timing constraints are satisfied.
 */
void cover_execute_action(zigbee_cover_cluster *cluster, cover_action_t action) {
    // COVER_ACTION_NONE is invalid here.
    if (action == COVER_ACTION_NONE) {
        return;
    }

    // If we're already at the target position, there's no need to move or update attributes.
    if (action == cluster->motor_position) {
        return;
    }

    // If we're already stopped, no need to toggle relays or update attributes.
    if (action == COVER_ACTION_STOP && cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
        return;
    }

    // If we're moving let's cancel any pending stop/position-update tasks and update the position.
    // We will either stop the movement completely or reschedule the necessary tasks.
    if (cluster->moving != ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
        cover_cancel_movement_tasks(cluster);
        cover_update_position(cluster);
    }

    // Handle the stop case
    if (action == COVER_ACTION_STOP) {
        relay_off(cluster->close_relay);
        relay_off(cluster->open_relay);
        cluster->last_switch_time = hal_millis();
        cluster->moving           = ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_WINDOW_COVERING,
                                            ZCL_ATTR_WINDOW_COVERING_MOVING);
        return;
    }

    // Update relays if necessary.
    uint8_t direction = (action > cluster->motor_position)
        ? ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING
        : ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING;
    if (direction != cluster->moving) {
        relay_t *open_relay  = cluster->motor_reversal ? cluster->close_relay : cluster->open_relay;
        relay_t *close_relay = cluster->motor_reversal ? cluster->open_relay : cluster->close_relay;
        if (direction == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING) {
            relay_on(open_relay);
            relay_off(close_relay);
        } else if (direction == ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING) {
            relay_off(open_relay);
            relay_on(close_relay);
        } else {
            relay_off(open_relay);
            relay_off(close_relay);
        }

        cluster->last_switch_time = hal_millis();
        cluster->moving           = direction;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_WINDOW_COVERING,
                                            ZCL_ATTR_WINDOW_COVERING_MOVING);
    }

    cluster->target_motor_position = action;
    // In manual mode, position tracking is disabled. Snap the position attribute
    // immediately to the endpoint and schedule a fixed safety timeout instead of the
    // travel-time-based stop.
    if (cover_is_manual_mode(cluster)) {
        cluster->motor_position = (direction == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING)
                                  ? MAX_MOTOR_POSITION : 0;
        cluster->position = (direction == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING) ? 100 : 0;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_WINDOW_COVERING,
                                            ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE);
        hal_tasks_schedule(&cluster->stop_task, MANUAL_MODE_TIMEOUT_MS);
        return;
    }

    // Schedule the stop task for the travel duration plus OVERRUN_DURATION_MS when the target
    // is a physical end position.
    cluster->start_motor_position = cluster->motor_position;
    cluster->movement_start_time  = hal_millis();
    uint32_t position_delta;
    if (direction == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING) {
        position_delta = cluster->target_motor_position - cluster->motor_position;
    } else {
        position_delta = cluster->motor_position - cluster->target_motor_position;
    }

    uint32_t travel_time_ms = cover_travel_time(cluster, direction);
    uint32_t duration_ms    = (position_delta * travel_time_ms) / MAX_MOTOR_POSITION;
    if (cluster->target_motor_position == 0 ||
        cluster->target_motor_position == MAX_MOTOR_POSITION) {
        duration_ms += OVERRUN_DURATION_MS;
    }

    hal_tasks_schedule(&cluster->stop_task, duration_ms);
    cover_schedule_next_update(cluster);
}

void cover_auto_stop_handler(void *arg) {
    zigbee_cover_cluster *cluster = (zigbee_cover_cluster *)arg;

    cover_execute_action(cluster, COVER_ACTION_STOP);

    // Snap exactly to target
    cluster->motor_position = cluster->target_motor_position;

    uint8_t new_cover_pos = motor_to_cover_position(cluster);
    if (new_cover_pos != cluster->position) {
        cluster->position = new_cover_pos;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_WINDOW_COVERING,
                                            ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE);
    }
}

void cover_defer_action(zigbee_cover_cluster *cluster, cover_action_t action, uint32_t delay) {
    hal_tasks_unschedule(&cluster->delay_task);
    cluster->pending_action = action;
    hal_tasks_schedule(&cluster->delay_task, delay);
}

/**
 * Dispatches an action with motor protection timing.
 *
 * Safe, high-level entry point for all movement commands. Enforces
 * RELAY_MIN_SWITCH_TIME_MS between relay state changes to protect the
 * motor and relay contacts from arc damage and mechanical shock.
 */
void cover_dispatch_action(zigbee_cover_cluster *cluster, cover_action_t action) {
    if (action == COVER_ACTION_NONE) {
        return;
    }

    uint32_t elapsed = hal_millis() - cluster->last_switch_time;
    if (action == COVER_ACTION_STOP) {
        // If already stopped, no action is needed. Cancel any pending deferred action.
        if (cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
            if (cluster->pending_action != COVER_ACTION_NONE) {
                hal_tasks_unschedule(&cluster->delay_task);
                cluster->pending_action = COVER_ACTION_NONE;
            }
            return;
        }

        // If within the minimum switch time, delay the stop. Otherwise, stop immediately.
        if (elapsed < RELAY_MIN_SWITCH_TIME_MS) {
            cover_defer_action(cluster, COVER_ACTION_STOP, RELAY_MIN_SWITCH_TIME_MS - elapsed);
        } else {
            cover_execute_action(cluster, COVER_ACTION_STOP);
        }
    } else {
        uint8_t direction = action > cluster->motor_position
            ? ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING
            : ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING;

        // If already moving in the correct direction, cancel any pending deferred action
        // (e.g. a reversal that has not yet executed) and delegate to cover_execute_action,
        // which handles all task rescheduling.
        if (cluster->moving == direction) {
            if (cluster->pending_action != COVER_ACTION_NONE) {
                hal_tasks_unschedule(&cluster->delay_task);
                cluster->pending_action = COVER_ACTION_NONE;
            }
            cover_execute_action(cluster, action);
            return;
        }

        if (elapsed < RELAY_MIN_SWITCH_TIME_MS) {
            cover_defer_action(cluster, action, RELAY_MIN_SWITCH_TIME_MS - elapsed);
        } else if (cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED) {
            // Already stopped and outside the minimum switch time, so start moving immediately.
            cover_execute_action(cluster, action);
        } else {
            // Moving in the opposite direction and outside the minimum switch time,
            // so stop immediately and defer the new action.
            cover_execute_action(cluster, COVER_ACTION_STOP);
            cover_defer_action(cluster, action, RELAY_MIN_SWITCH_TIME_MS);
        }
    }
}

void cover_open(zigbee_cover_cluster *cluster) {
    cover_dispatch_action(cluster, MAX_MOTOR_POSITION);
}

void cover_close(zigbee_cover_cluster *cluster) {
    cover_dispatch_action(cluster, 0);
}

void cover_stop(zigbee_cover_cluster *cluster) {
    cover_dispatch_action(cluster, COVER_ACTION_STOP);
}

void cover_delay_handler(void *arg) {
    zigbee_cover_cluster *cluster = (zigbee_cover_cluster *)arg;

    cover_action_t action = cluster->pending_action;

    cluster->pending_action = COVER_ACTION_NONE;
    cover_dispatch_action(cluster, action);
}

// ============================================================================
// NVM Persistence
// ============================================================================

void cover_cluster_store_attrs_to_nv(zigbee_cover_cluster *cluster) {
    nv_config_buffer.motor_reversal  = cluster->motor_reversal;
    nv_config_buffer.open_time       = cluster->open_time;
    nv_config_buffer.close_time      = cluster->close_time;
    nv_config_buffer.open_deadzone   = cluster->open_deadzone;
    nv_config_buffer.closed_deadzone = cluster->closed_deadzone;
    nv_config_buffer.motor_position  = cluster->motor_position;

    hal_nvm_write(NV_ITEM_COVER_CONFIG(cluster->cover_idx),
                  sizeof(zigbee_cover_cluster_config),
                  (uint8_t *)&nv_config_buffer);
}

void cover_cluster_load_attrs_from_nv(zigbee_cover_cluster *cluster) {
    hal_nvm_status_t st = hal_nvm_read(NV_ITEM_COVER_CONFIG(cluster->cover_idx),
                                       sizeof(zigbee_cover_cluster_config),
                                       (uint8_t *)&nv_config_buffer);

    if (st != HAL_NVM_SUCCESS) {
        return;
    }

    cluster->motor_reversal  = nv_config_buffer.motor_reversal;
    cluster->open_time       = nv_config_buffer.open_time;
    cluster->close_time      = nv_config_buffer.close_time;
    cluster->open_deadzone   = nv_config_buffer.open_deadzone;
    cluster->closed_deadzone = nv_config_buffer.closed_deadzone;
    cluster->motor_position  = nv_config_buffer.motor_position;

    cluster->position = motor_to_cover_position(cluster);
}

// ============================================================================
// Attribute & Command Handlers
// ============================================================================

void cover_cluster_on_write_attr(zigbee_cover_cluster *cluster, uint16_t attribute_id) {
    if (attribute_id == ZCL_ATTR_WINDOW_COVERING_MOTOR_REVERSAL) {
        cover_dispatch_action(cluster, COVER_ACTION_STOP);
    }

    // Deadzone values are percentages: cap each at 50% so we don't exceed 100%
    // even if both are set to their maximum.
    if (cluster->open_deadzone > 50) cluster->open_deadzone = 50;
    if (cluster->closed_deadzone > 50) cluster->closed_deadzone = 50;

    cover_cluster_store_attrs_to_nv(cluster);
}

void cover_cluster_callback_attr_write_trampoline(uint8_t endpoint, uint16_t attribute_id) {
    cover_cluster_on_write_attr(cover_cluster_by_endpoint[endpoint], attribute_id);
}

hal_zigbee_cmd_result_t cover_cluster_callback(zigbee_cover_cluster *cluster,
                                               uint8_t command_id,
                                               void *cmd_payload,
                                               uint16_t cmd_payload_len) {
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
        if (cover_is_manual_mode(cluster)) {
            break;
        }
        if (cmd_payload == NULL || cmd_payload_len < 1) {
            return HAL_ZIGBEE_MALFORMED_COMMAND;
        }

        uint8_t target_percentage = *((uint8_t *)cmd_payload);
        if (target_percentage > 100) {
            return HAL_ZIGBEE_INVALID_VALUE;
        }

        cover_dispatch_action(cluster, cover_to_motor_position(cluster, target_percentage));
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
    cluster->moving          = ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED;
    cluster->motor_reversal  = 0;
    cluster->open_time       = 300;
    cluster->close_time      = 0;
    cluster->open_deadzone   = 0;
    cluster->closed_deadzone = 0;
    cluster->position        = 50;

    // State
    cluster->last_switch_time      = 0;
    cluster->pending_action        = COVER_ACTION_NONE;
    cluster->motor_position        = MAX_MOTOR_POSITION / 2;
    cluster->movement_start_time   = 0;
    cluster->start_motor_position  = 0;
    cluster->target_motor_position = 0;

    hal_tasks_init(&cluster->delay_task);
    cluster->delay_task.handler = cover_delay_handler;
    cluster->delay_task.arg     = cluster;

    hal_tasks_init(&cluster->stop_task);
    cluster->stop_task.handler = cover_auto_stop_handler;
    cluster->stop_task.arg     = cluster;

    hal_tasks_init(&cluster->update_task);
    cluster->update_task.handler = cover_update_handler;
    cluster->update_task.arg     = cluster;
}

void cover_cluster_add_to_endpoint(zigbee_cover_cluster *cluster, hal_zigbee_endpoint *endpoint) {
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
               cluster->position);
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
               cluster->open_time);
    SETUP_ATTR(5,
               ZCL_ATTR_WINDOW_COVERING_CLOSE_TIME,
               ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE,
               cluster->close_time);
    SETUP_ATTR(6,
               ZCL_ATTR_WINDOW_COVERING_OPEN_DEADZONE,
               ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE,
               cluster->open_deadzone);
    SETUP_ATTR(7,
               ZCL_ATTR_WINDOW_COVERING_CLOSED_DEADZONE,
               ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE,
               cluster->closed_deadzone);

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_WINDOW_COVERING;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 8;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    = cover_cluster_callback_trampoline;
    endpoint->cluster_count++;
}
