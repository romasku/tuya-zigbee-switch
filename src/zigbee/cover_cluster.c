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

// Current lift position percentage - required by ZCL spec for liftable covers.
// Hardcoded to 50 until position calculation/control is implemented.
static uint8_t cover_position = 50;

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
void cover_apply_movement(zigbee_cover_cluster *cluster, uint8_t moving)
{
  relay_t *open_relay  = cluster->motor_reversal ? cluster->close_relay : cluster->open_relay;
  relay_t *close_relay = cluster->motor_reversal ? cluster->open_relay : cluster->close_relay;

  cluster->last_switch_time = hal_millis();
  if (moving == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING)
  {
    relay_on(open_relay);
    relay_off(close_relay);
    cluster->moving = ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING;
  }
  else if (moving == ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING)
  {
    relay_off(open_relay);
    relay_on(close_relay);
    cluster->moving = ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING;
  }
  else
  {
    relay_off(open_relay);
    relay_off(close_relay);
    cluster->moving = ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED;
  }

  hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                      ZCL_CLUSTER_WINDOW_COVERING,
                                      ZCL_ATTR_WINDOW_COVERING_MOVING);
}

void cover_schedule_movement(zigbee_cover_cluster *cluster, uint8_t moving, uint32_t delay)
{
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
void cover_request_movement(zigbee_cover_cluster *cluster, uint8_t moving)
{
  // Ignore duplicate requests and cancel any pending delayed operation. This is especially
  // important for some cover switches with stop buttons. Their stop button closes both contacts,
  // so pressing it while moving might initially appear as a repeated/reversal command before the
  // stop command arrives. Canceling pending operations ensures we handle this sequence correctly.
  if (moving == cluster->moving)
  {
    if (cluster->has_pending_movement)
    {
      hal_tasks_unschedule(&cluster->delay_task);
    }

    return;
  }

  // Enforce motor protection delay. Minimum time must elapse between relay state changes.
  uint32_t elapsed = hal_millis() - cluster->last_switch_time;
  if (elapsed < RELAY_MIN_SWITCH_TIME_MS)
  {
    cover_schedule_movement(cluster, moving, RELAY_MIN_SWITCH_TIME_MS - elapsed);
    return;
  }

  // Direct transitions to/from STOP can be applied immediately. Direction reversals require
  // stopping first to avoid damage to the motor and the relays.
  if (moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED ||
      cluster->moving == ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED)
  {
    cover_apply_movement(cluster, moving);
  }
  else
  {
    cover_apply_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED);
    cover_schedule_movement(cluster, moving, RELAY_MIN_SWITCH_TIME_MS);
  }
}

void cover_open(zigbee_cover_cluster *cluster)
{
  cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING);
}

void cover_close(zigbee_cover_cluster *cluster)
{
  cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING);
}

void cover_stop(zigbee_cover_cluster *cluster)
{
  cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED);
}

void cover_delay_handler(void *arg)
{
  zigbee_cover_cluster *cluster = (zigbee_cover_cluster *)arg;

  cover_request_movement(cluster, cluster->pending_movement);
}

// ============================================================================
// NVM Persistence
// ============================================================================

void cover_cluster_store_attrs_to_nv(zigbee_cover_cluster *cluster)
{
  nv_config_buffer.motor_reversal = cluster->motor_reversal;

  hal_nvm_write(NV_ITEM_COVER_CONFIG(cluster->cover_idx),
                sizeof(zigbee_cover_cluster_config),
                (uint8_t *)&nv_config_buffer);
}

void cover_cluster_load_attrs_from_nv(zigbee_cover_cluster *cluster)
{
  hal_nvm_status_t st = hal_nvm_read(NV_ITEM_COVER_CONFIG(cluster->cover_idx),
                                     sizeof(zigbee_cover_cluster_config),
                                     (uint8_t *)&nv_config_buffer);

  if (st != HAL_NVM_SUCCESS)
  {
    return;
  }

  cluster->motor_reversal = nv_config_buffer.motor_reversal;
}

// ============================================================================
// Attribute & Command Handlers
// ============================================================================

void cover_cluster_on_write_attr(zigbee_cover_cluster *cluster, uint16_t attribute_id)
{
  if (attribute_id == ZCL_ATTR_WINDOW_COVERING_MOTOR_REVERSAL)
  {
    cover_request_movement(cluster, ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED);
  }

  cover_cluster_store_attrs_to_nv(cluster);
}

void cover_cluster_callback_attr_write_trampoline(uint8_t endpoint, uint16_t attribute_id)
{
  cover_cluster_on_write_attr(cover_cluster_by_endpoint[endpoint], attribute_id);
}

hal_zigbee_cmd_result_t cover_cluster_callback(zigbee_cover_cluster *cluster,
                                               uint8_t command_id,
                                               void *cmd_payload)
{
  switch (command_id)
  {
  case ZCL_CMD_WINDOW_COVERING_UP_OPEN:
    cover_open(cluster);
    break;
  case ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE:
    cover_close(cluster);
    break;
  case ZCL_CMD_WINDOW_COVERING_STOP:
    cover_stop(cluster);
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
                                                          void *cmd_payload)
{
  return(cover_cluster_callback(cover_cluster_by_endpoint[endpoint], command_id, cmd_payload));
}

// ============================================================================
// Initialization
// ============================================================================

void cover_cluster_init(zigbee_cover_cluster *cluster)
{
  // Attributes
  cluster->moving         = ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED;
  cluster->motor_reversal = 0;

  // State
  cluster->last_switch_time     = 0;
  cluster->pending_movement     = 0;
  cluster->has_pending_movement = 0;

  hal_tasks_init(&cluster->delay_task);
  cluster->delay_task.handler = cover_delay_handler;
  cluster->delay_task.arg     = cluster;
}

void cover_cluster_add_to_endpoint(zigbee_cover_cluster *cluster, hal_zigbee_endpoint *endpoint)
{
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
             cover_position);
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

  endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_WINDOW_COVERING;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 4;
  endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
  endpoint->clusters[endpoint->cluster_count].is_server       = 1;
  endpoint->clusters[endpoint->cluster_count].cmd_callback    = cover_cluster_callback_trampoline;
  endpoint->cluster_count++;
}
