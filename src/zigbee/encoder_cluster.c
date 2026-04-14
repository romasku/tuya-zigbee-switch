#include "encoder_cluster.h"
#include "base_components/relay.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"

#include "hal/printf_selector.h"
#include "hal/system.h"
#include "hal/tasks.h"
#include "relay_cluster.h"
#include "zigbee_commands.h"

const uint8_t multistate_out_of_service_2 = 0;
const uint8_t multistate_flags_2 = 0;
const uint16_t multistate_num_of_states_2 = 3;

#define MULTISTATE_NOT_PRESSED 0
#define MULTISTATE_PRESS 1
#define MULTISTATE_LONG_PRESS 2
#define MULTISTATE_POSITION_ON 3
#define MULTISTATE_POSITION_OFF 4

extern zigbee_relay_cluster relay_clusters[];
extern uint8_t relay_clusters_cnt;
extern zigbee_encoder_cluster encoder_clusters[];
extern uint8_t encoder_clusters_cnt;

void encoder_cluster_on_button_press(zigbee_encoder_cluster *cluster);
void encoder_cluster_on_button_release(zigbee_encoder_cluster *cluster);
void encoder_cluster_on_button_long_press(zigbee_encoder_cluster *cluster);
void encoder_cluster_on_rotate_cw(zigbee_encoder_cluster *cluster);

static bool encoder_cluster_has_valid_relay(
    const zigbee_encoder_cluster *cluster);

zigbee_encoder_cluster *encoder_cluster_by_endpoint[10];

void update_encoder_clusters()
{
}

void encoder_cluster_on_write_attr(zigbee_encoder_cluster *cluster,
                                   uint16_t attribute_id);

void encoder_cluster_report_action(zigbee_encoder_cluster *cluster);

void encoder_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                    uint16_t attribute_id)
{
  encoder_cluster_on_write_attr(encoder_cluster_by_endpoint[endpoint],
                                attribute_id);
}

void encoder_cluster_add_to_endpoint(zigbee_encoder_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint)
{
  encoder_cluster_by_endpoint[endpoint->endpoint] = cluster;
  cluster->endpoint = endpoint->endpoint;

  cluster->encoder->on_press =
      (ev_encoder_callback_t)encoder_cluster_on_button_press;
  cluster->encoder->on_rotate_cw = (ev_encoder_callback_t)encoder_cluster_on_rotate_cw;
  cluster->encoder->callback_param = cluster;

  SETUP_ATTR(0, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE, ZCL_DATA_TYPE_ENUM8,
             ATTR_READONLY, cluster->mode);
  SETUP_ATTR(1, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
             ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE, cluster->action);
  SETUP_ATTR(2, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE, ZCL_DATA_TYPE_ENUM8,
             ATTR_WRITABLE, cluster->mode);
  SETUP_ATTR(3, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
             ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE, cluster->relay_mode);
  SETUP_ATTR(4, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
             ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE, cluster->relay_index);
  // SETUP_ATTR(5, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
  //            ZCL_DATA_TYPE_UINT16, ATTR_WRITABLE,
  //            cluster->button->long_press_duration_ms);
  SETUP_ATTR(6, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
             ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE, cluster->level_move_rate);
  SETUP_ATTR(7, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
             ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE, cluster->binded_mode);

  // Configuration
  endpoint->clusters[endpoint->cluster_count].cluster_id =
      ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 8;
  endpoint->clusters[endpoint->cluster_count].attributes = cluster->attr_infos;
  endpoint->clusters[endpoint->cluster_count].is_server = 1;
  endpoint->cluster_count++;

  // Output ON OFF to bind to other devices
  endpoint->clusters[endpoint->cluster_count].cluster_id = ZCL_CLUSTER_ON_OFF;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
  endpoint->clusters[endpoint->cluster_count].attributes = NULL;
  endpoint->clusters[endpoint->cluster_count].is_server = 0;
  endpoint->cluster_count++;

  SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 0,
                       ZCL_ATTR_MULTISTATE_INPUT_NUMBER_OF_STATES,
                       ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                       multistate_num_of_states_2);
  SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 1,
                       ZCL_ATTR_MULTISTATE_INPUT_OUT_OF_SERVICE,
                       ZCL_DATA_TYPE_BOOLEAN, ATTR_READONLY,
                       multistate_out_of_service_2);
  SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 2,
                       ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
                       ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                       cluster->multistate_state);
  SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 3,
                       ZCL_ATTR_MULTISTATE_INPUT_STATUS_FLAGS,
                       ZCL_DATA_TYPE_BITMAP8, ATTR_READONLY, multistate_flags_2);

  // Output
  endpoint->clusters[endpoint->cluster_count].cluster_id =
      ZCL_CLUSTER_MULTISTATE_INPUT_BASIC;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 4;
  endpoint->clusters[endpoint->cluster_count].attributes =
      cluster->multistate_attr_infos;
  endpoint->clusters[endpoint->cluster_count].is_server = 1;
  endpoint->cluster_count++;

  // Output Level for other devices
  endpoint->clusters[endpoint->cluster_count].cluster_id =
      ZCL_CLUSTER_LEVEL_CONTROL;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
  endpoint->clusters[endpoint->cluster_count].attributes = NULL;
  endpoint->clusters[endpoint->cluster_count].is_server = 0;
  endpoint->cluster_count++;
}

// Perform the relay action for ON position (position 1 in ZCL docs)
void encoder_cluster_relay_action_on(zigbee_encoder_cluster *cluster)
{
}

// Perform the relay action for OFF position (position 2 in ZCL docs)
void encoder_cluster_relay_action_off(zigbee_encoder_cluster *cluster)
{
}

// Send OnOff command to binded device based on ON position (position 1 in
// ZCL docs)
void encoder_cluster_binding_action_on(zigbee_encoder_cluster *cluster)
{
  printf("1\r\n");

  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED)
  {
    return;
  }

  uint8_t cmd_id;

  switch (cluster->action)
  {
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
    cmd_id = ZCL_CMD_ONOFF_ON;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
    cmd_id = ZCL_CMD_ONOFF_OFF;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
    cmd_id = ZCL_CMD_ONOFF_TOGGLE;
    break;

  default:
    return;
  }

  printf("Building and sending actions to binds\r\n");

  hal_zigbee_cmd c = build_onoff_cmd(cluster->endpoint, cmd_id);
  hal_zigbee_send_cmd_to_bindings(&c);
}

// Send OnOff command to binded device based on OFF position (position 2 in
// ZCL docs)
void encoder_cluster_binding_action_off(zigbee_encoder_cluster *cluster)
{
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED)
  {
    return;
  }

  uint8_t cmd_id;

  switch (cluster->action)
  {
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
    cmd_id = ZCL_CMD_ONOFF_OFF;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
    cmd_id = ZCL_CMD_ONOFF_ON;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
    cmd_id = ZCL_CMD_ONOFF_TOGGLE;
    break;

  default:
    return;
  }

  hal_zigbee_cmd c = build_onoff_cmd(cluster->endpoint, cmd_id);
  hal_zigbee_send_cmd_to_bindings(&c);
}

void encoder_cluster_level_stop(zigbee_encoder_cluster *cluster)
{
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED)
  {
    return;
  }

  hal_zigbee_cmd c = build_level_stop_onoff_cmd(cluster->endpoint);
  hal_zigbee_send_cmd_to_bindings(&c);
}

void encoder_cluster_level_control(zigbee_encoder_cluster *cluster)
{
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED)
  {
    return;
  }

  hal_zigbee_cmd c = build_level_move_onoff_cmd(cluster->endpoint,
                                                cluster->level_move_direction,
                                                cluster->level_move_rate);
  hal_zigbee_send_cmd_to_bindings(&c);

  if (cluster->level_move_direction == ZCL_LEVEL_MOVE_DOWN)
  {
    cluster->level_move_direction = ZCL_LEVEL_MOVE_UP;
  }
  else
  {
    cluster->level_move_direction = ZCL_LEVEL_MOVE_DOWN;
  }
}

void encoder_cluster_level_control_step(zigbee_encoder_cluster *cluster)
{
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED)
  {
    return;
  }

  printf("Sending Level Step Command\r\n");

  hal_zigbee_cmd c = build_level_step_cmd(cluster->endpoint);
  hal_zigbee_send_cmd_to_bindings(&c);
}

void encoder_cluster_on_button_press(zigbee_encoder_cluster *cluster)
{
  printf("Encoder Cluster button pressed cb triggered\r\n");
  encoder_cluster_binding_action_on(cluster);
  cluster->multistate_state = MULTISTATE_POSITION_ON;
  hal_zigbee_notify_attribute_changed(
      cluster->endpoint, ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
      ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

void encoder_cluster_on_write_attr(zigbee_encoder_cluster *cluster,
                                   uint16_t attribute_id)
{
  printf("Index at write attr: %d\r\n", cluster->switch_idx);
}

void encoder_cluster_on_rotate_cw(zigbee_encoder_cluster *cluster)
{
  printf("Encoder Cluster rotate cw cb triggered\r\n");
  encoder_cluster_level_control_step(cluster);
}