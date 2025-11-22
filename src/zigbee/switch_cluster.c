
#include "switch_cluster.h"
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

#define MULTI_PRESS_CNT_TO_RESET 10

const uint8_t multistate_out_of_service = 0;
const uint8_t multistate_flags = 0;
const uint16_t multistate_num_of_states = 3;

#define MULTISTATE_NOT_PRESSED 0
#define MULTISTATE_PRESS 1
#define MULTISTATE_LONG_PRESS 2
#define MULTISTATE_POSITION_ON 3
#define MULTISTATE_POSITION_OFF 4

extern zigbee_relay_cluster relay_clusters[];
extern uint8_t relay_clusters_cnt;

void switch_cluster_on_button_press(zigbee_switch_cluster *cluster);
void switch_cluster_on_button_release(zigbee_switch_cluster *cluster);
void switch_cluster_on_button_long_press(zigbee_switch_cluster *cluster);
void switch_cluster_on_button_multi_press(zigbee_switch_cluster *cluster,
                                          uint8_t press_count);

zigbee_switch_cluster *switch_cluster_by_endpoint[10];

void switch_cluster_store_attrs_to_nv(zigbee_switch_cluster *cluster);
void switch_cluster_load_attrs_from_nv(zigbee_switch_cluster *cluster);
void switch_cluster_on_write_attr(zigbee_switch_cluster *cluster,
                                  uint16_t attribute_id);

void switch_cluster_report_action(zigbee_switch_cluster *cluster);

void switch_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                   uint16_t attribute_id) {
  switch_cluster_on_write_attr(switch_cluster_by_endpoint[endpoint],
                               attribute_id);
}

void switch_cluster_add_to_endpoint(zigbee_switch_cluster *cluster,
                                    hal_zigbee_endpoint *endpoint) {
  switch_cluster_by_endpoint[endpoint->endpoint] = cluster;
  cluster->endpoint = endpoint->endpoint;
  switch_cluster_load_attrs_from_nv(cluster);

  cluster->button->on_press =
      (ev_button_callback_t)switch_cluster_on_button_press;
  cluster->button->on_release =
      (ev_button_callback_t)switch_cluster_on_button_release;
  cluster->button->on_long_press =
      (ev_button_callback_t)switch_cluster_on_button_long_press;
  cluster->button->on_multi_press =
      (ev_button_multi_press_callback_t)switch_cluster_on_button_multi_press;
  cluster->button->callback_param = cluster;

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
  SETUP_ATTR(5, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
             ZCL_DATA_TYPE_UINT16, ATTR_WRITABLE,
             cluster->button->long_press_duration_ms);
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
                       multistate_num_of_states);
  SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 1,
                       ZCL_ATTR_MULTISTATE_INPUT_OUT_OF_SERVICE,
                       ZCL_DATA_TYPE_BOOLEAN, ATTR_READONLY,
                       multistate_out_of_service);
  SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 2,
                       ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
                       ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                       cluster->multistate_state);
  SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 3,
                       ZCL_ATTR_MULTISTATE_INPUT_STATUS_FLAGS,
                       ZCL_DATA_TYPE_BITMAP8, ATTR_READONLY, multistate_flags);

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
void switch_cluster_relay_action_on(zigbee_switch_cluster *cluster) {
  zigbee_relay_cluster *relay_cluster =
      &relay_clusters[cluster->relay_index - 1];

  switch (cluster->action) {
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
    relay_cluster_on(relay_cluster);
    break;
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
    relay_cluster_off(relay_cluster);
    break;
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
    relay_cluster_toggle(relay_cluster);
    break;
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC:
    relay_cluster_toggle(relay_cluster);
    break;
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE:
    relay_cluster_toggle(relay_cluster);
    break;
  }
}

// Perform the relay action for OFF position (position 2 in ZCL docs)
void switch_cluster_relay_action_off(zigbee_switch_cluster *cluster) {
  zigbee_relay_cluster *relay_cluster =
      &relay_clusters[cluster->relay_index - 1];

  switch (cluster->action) {
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
    relay_cluster_off(relay_cluster);
    break;
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
    relay_cluster_on(relay_cluster);
    break;
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
    relay_cluster_toggle(relay_cluster);
    break;
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC:
    relay_cluster_toggle(relay_cluster);
    break;
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE:
    relay_cluster_toggle(relay_cluster);
    break;
  }
}

// Send OnOff command to binded device based on ON position (position 1 in
// ZCL docs)
void switch_cluster_binding_action_on(zigbee_switch_cluster *cluster) {
  zigbee_relay_cluster *relay_cluster =
      &relay_clusters[cluster->relay_index - 1];

  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
    return;
  }

  uint8_t cmd_id;

  switch (cluster->action) {
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
    cmd_id = ZCL_CMD_ONOFF_ON;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
    cmd_id = ZCL_CMD_ONOFF_OFF;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
    cmd_id = ZCL_CMD_ONOFF_TOGGLE;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC:
    cmd_id = (relay_cluster->relay->on) ? ZCL_CMD_ONOFF_ON : ZCL_CMD_ONOFF_OFF;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE:
    cmd_id = (relay_cluster->relay->on) ? ZCL_CMD_ONOFF_OFF : ZCL_CMD_ONOFF_ON;
    break;

  default:
    return;
  }

  hal_zigbee_cmd c = build_onoff_cmd(cluster->endpoint, cmd_id);
  hal_zigbee_send_cmd_to_bindings(&c);
}

// Send OnOff command to binded device based on OFF position (position 2 in
// ZCL docs)
void switch_cluster_binding_action_off(zigbee_switch_cluster *cluster) {
  zigbee_relay_cluster *relay_cluster =
      &relay_clusters[cluster->relay_index - 1];

  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
    return;
  }

  uint8_t cmd_id;

  switch (cluster->action) {
  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
    cmd_id = ZCL_CMD_ONOFF_OFF;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
    cmd_id = ZCL_CMD_ONOFF_ON;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
    cmd_id = ZCL_CMD_ONOFF_TOGGLE;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC:
    cmd_id = (relay_cluster->relay->on) ? ZCL_CMD_ONOFF_ON : ZCL_CMD_ONOFF_OFF;
    break;

  case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE:
    cmd_id = (relay_cluster->relay->on) ? ZCL_CMD_ONOFF_OFF : ZCL_CMD_ONOFF_ON;
    break;

  default:
    return;
  }

  hal_zigbee_cmd c = build_onoff_cmd(cluster->endpoint, cmd_id);
  hal_zigbee_send_cmd_to_bindings(&c);
}

void switch_cluster_level_stop(zigbee_switch_cluster *cluster) {
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
    return;
  }

  hal_zigbee_cmd c = build_level_stop_onoff_cmd(cluster->endpoint);
  hal_zigbee_send_cmd_to_bindings(&c);
}

void switch_cluster_level_control(zigbee_switch_cluster *cluster) {
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
    return;
  }

  hal_zigbee_cmd c = build_level_move_onoff_cmd(cluster->endpoint,
                                                cluster->level_move_direction,
                                                cluster->level_move_rate);
  hal_zigbee_send_cmd_to_bindings(&c);

  if (cluster->level_move_direction == ZCL_LEVEL_MOVE_DOWN) {
    cluster->level_move_direction = ZCL_LEVEL_MOVE_UP;
  } else {
    cluster->level_move_direction = ZCL_LEVEL_MOVE_DOWN;
  }
}

void switch_cluster_on_button_press(zigbee_switch_cluster *cluster) {

  if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
    // Toggle does not support modes (RISE, SHORT, LONG)
    if (cluster->relay_mode != ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED) {
      switch_cluster_relay_action_on(cluster);
    }
    switch_cluster_binding_action_on(cluster);
    cluster->multistate_state = MULTISTATE_POSITION_ON;
    hal_zigbee_notify_attribute_changed(
        cluster->endpoint, ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
    return;
  }

  if (cluster->relay_mode == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE) {
    switch_cluster_relay_action_on(cluster);
  }

  if (cluster->binded_mode == ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE) {
    switch_cluster_binding_action_on(cluster);
  }

  cluster->multistate_state = MULTISTATE_PRESS;
  hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                      ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                      ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

void switch_cluster_on_button_release(zigbee_switch_cluster *cluster) {

  if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
    // Toggle does not support modes (RISE, SHORT, LONG)
    if (cluster->relay_mode != ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED) {
      switch_cluster_relay_action_off(cluster);
    }
    switch_cluster_binding_action_off(cluster);
    cluster->multistate_state = MULTISTATE_POSITION_OFF;
    hal_zigbee_notify_attribute_changed(
        cluster->endpoint, ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
    return;
  }

  if (cluster->multistate_state != MULTISTATE_LONG_PRESS) {
    if (cluster->relay_mode == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT) {
      switch_cluster_relay_action_on(cluster);
    }
    if (cluster->binded_mode == ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT) {
      switch_cluster_binding_action_on(cluster);
    }
  } else {
    // This is end of long press, send zcl_level stop
    switch_cluster_level_stop(cluster);
  }

  cluster->multistate_state = MULTISTATE_NOT_PRESSED;
  hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                      ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                      ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

void switch_cluster_on_button_long_press(zigbee_switch_cluster *cluster) {
  if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
    // Toggle does not support modes (RISE, SHORT, LONG)
    return;
  }

  zigbee_relay_cluster *relay_cluster =
      &relay_clusters[cluster->relay_index - 1];

  if (cluster->relay_mode == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG) {
    relay_cluster_toggle(relay_cluster);
  }

  if (cluster->binded_mode == ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG) {
    switch_cluster_binding_action_on(cluster);
  }

  switch_cluster_level_control(cluster);

  cluster->multistate_state = MULTISTATE_LONG_PRESS;
  hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                      ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                      ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

hal_task_t restart_task;

void reset_tasks_handler(void *arg) { hal_system_reset(); }

void switch_cluster_on_button_multi_press(zigbee_switch_cluster *cluster,
                                          uint8_t press_count) {
  if (press_count > MULTI_PRESS_CNT_TO_RESET) {
    hal_factory_reset();
  }
}

void synchronize_multistate_state(zigbee_switch_cluster *cluster) {
  if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
    if (cluster->button->pressed) {
      cluster->multistate_state = MULTISTATE_POSITION_ON;
    } else {
      cluster->multistate_state = MULTISTATE_POSITION_OFF;
    }
  } else {
    if (cluster->button->long_pressed) {
      cluster->multistate_state = MULTISTATE_LONG_PRESS;
    } else if (cluster->button->pressed) {
      cluster->multistate_state = MULTISTATE_PRESS;
    } else {
      cluster->multistate_state = MULTISTATE_NOT_PRESSED;
    }
  }
  hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                      ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                      ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

void switch_cluster_on_write_attr(zigbee_switch_cluster *cluster,
                                  uint16_t attribute_id) {
  printf("Index at write attr: %d\r\n", cluster->switch_idx);
  if (attribute_id == ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX) {
    if (cluster->relay_index < 1 || cluster->relay_index > relay_clusters_cnt) {
      cluster->relay_index = 1;
    }
  }
  if (attribute_id == ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE) {
    synchronize_multistate_state(cluster);
    if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY_NC) {
      cluster->button->pressed_when_high = 1;
    } else {
      cluster->button->pressed_when_high = 0;
    }
  }
  switch_cluster_store_attrs_to_nv(cluster);
}

zigbee_switch_cluster_config nv_config_buffer;

void switch_cluster_store_attrs_to_nv(zigbee_switch_cluster *cluster) {

  nv_config_buffer.action = cluster->action;
  nv_config_buffer.mode = cluster->mode;
  nv_config_buffer.relay_index = cluster->relay_index;
  nv_config_buffer.relay_mode = cluster->relay_mode;
  nv_config_buffer.button_long_press_duration =
      cluster->button->long_press_duration_ms;
  nv_config_buffer.level_move_rate = cluster->level_move_rate;
  nv_config_buffer.binded_mode = cluster->binded_mode;
  hal_nvm_write(NV_ITEM_SWITCH_CLUSTER_DATA(cluster->switch_idx),
                sizeof(zigbee_switch_cluster_config),
                (uint8_t *)&nv_config_buffer);
}

void switch_cluster_load_attrs_from_nv(zigbee_switch_cluster *cluster) {
  hal_nvm_status_t st = hal_nvm_read(
      NV_ITEM_SWITCH_CLUSTER_DATA(cluster->switch_idx),
      sizeof(zigbee_switch_cluster_config), (uint8_t *)&nv_config_buffer);

  if (st != HAL_NVM_SUCCESS) {
    printf("No switch config in NV, using defaults\r\n");
    return;
  }
  cluster->action = nv_config_buffer.action;
  cluster->mode = nv_config_buffer.mode;
  cluster->relay_index = nv_config_buffer.relay_index;
  cluster->relay_mode = nv_config_buffer.relay_mode;
  cluster->button->long_press_duration_ms =
      nv_config_buffer.button_long_press_duration;
  cluster->level_move_rate = nv_config_buffer.level_move_rate;
  cluster->binded_mode = nv_config_buffer.binded_mode;
}
