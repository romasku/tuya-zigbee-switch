#include "relay_cluster.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"

hal_zigbee_cmd_result_t relay_cluster_callback(zigbee_relay_cluster *cluster,
                                               uint8_t command_id,
                                               void *cmd_payload);
hal_zigbee_cmd_result_t relay_cluster_callback_trampoline(uint8_t endpoint,
                                                          uint8_t cluster_id,
                                                          uint8_t command_id,
                                                          void *cmd_payload);

void relay_cluster_on_relay_change(zigbee_relay_cluster *cluster,
                                   uint8_t state);
void relay_cluster_on_write_attr(zigbee_relay_cluster *cluster,
                                 uint16_t attribute_id);

void relay_cluster_store_attrs_to_nv(zigbee_relay_cluster *cluster);
void relay_cluster_load_attrs_from_nv(zigbee_relay_cluster *cluster);
void relay_cluster_handle_startup_mode(zigbee_relay_cluster *cluster);

void sync_indicator_led(zigbee_relay_cluster *cluster);

zigbee_relay_cluster *relay_cluster_by_endpoint[10];

void relay_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                  uint16_t attribute_id) {
  relay_cluster_on_write_attr(relay_cluster_by_endpoint[endpoint],
                              attribute_id);
}

void update_relay_clusters() {
  for (int i = 0; i < 10; i++) {
    if (relay_cluster_by_endpoint[i] != NULL) {
      sync_indicator_led(relay_cluster_by_endpoint[i]);
    }
  }
}

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint) {
  relay_cluster_by_endpoint[endpoint->endpoint] = cluster;
  cluster->endpoint = endpoint->endpoint;
  relay_cluster_load_attrs_from_nv(cluster);

  cluster->relay->callback_param = cluster;
  cluster->relay->on_change = (relay_callback_t)relay_cluster_on_relay_change;

  relay_cluster_handle_startup_mode(cluster);
  sync_indicator_led(cluster);

  SETUP_ATTR(0, ZCL_ATTR_ONOFF, ZCL_DATA_TYPE_BOOLEAN, ATTR_READONLY,
             cluster->relay->on);
  SETUP_ATTR(1, ZCL_ATTR_START_UP_ONOFF, ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE,
             cluster->startup_mode);
  if (cluster->indicator_led != NULL) {
    SETUP_ATTR(2, ZCL_ATTR_ONOFF_INDICATOR_MODE, ZCL_DATA_TYPE_ENUM8,
               ATTR_WRITABLE, cluster->indicator_led_mode);
    SETUP_ATTR(3, ZCL_ATTR_ONOFF_INDICATOR_STATE, ZCL_DATA_TYPE_BOOLEAN,
               ATTR_WRITABLE, cluster->indicator_led->on);
  }

  endpoint->clusters[endpoint->cluster_count].cluster_id = ZCL_CLUSTER_ON_OFF;
  endpoint->clusters[endpoint->cluster_count].attribute_count =
      cluster->indicator_led != NULL ? 4 : 2;
  endpoint->clusters[endpoint->cluster_count].attributes = cluster->attr_infos;
  endpoint->clusters[endpoint->cluster_count].is_server = 1;
  endpoint->clusters[endpoint->cluster_count].cmd_callback =
      relay_cluster_callback_trampoline;
  endpoint->cluster_count++;
}

hal_zigbee_cmd_result_t relay_cluster_callback_trampoline(uint8_t endpoint,
                                                          uint8_t cluster_id,
                                                          uint8_t command_id,
                                                          void *cmd_payload) {
  return relay_cluster_callback(relay_cluster_by_endpoint[endpoint], command_id,
                                cmd_payload);
}

hal_zigbee_cmd_result_t relay_cluster_callback(zigbee_relay_cluster *cluster,
                                               uint8_t command_id,
                                               void *cmd_payload) {
  switch (command_id) {
  case ZCL_CMD_ONOFF_ON:
  case ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE:
    relay_cluster_on(cluster);
    break;

  case ZCL_CMD_ONOFF_OFF:
  case ZCL_CMD_OFF_WITH_EFFECT:
    relay_cluster_off(cluster);
    break;

  case ZCL_CMD_ONOFF_TOGGLE:
    relay_cluster_toggle(cluster);
    break;

  default:
    printf("Unknown command: %d\r\n", command_id);
    break;
  }
  return HAL_ZIGBEE_CMD_PROCESSED;
}

void sync_indicator_led(zigbee_relay_cluster *cluster) {
  if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_MANUAL ||
      cluster->indicator_led == NULL) {
    return;
  }

  uint8_t turn_on_led = cluster->relay->on;

  if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_OPPOSITE) {
    turn_on_led = !turn_on_led;
  }

  turn_on_led ? led_on(cluster->indicator_led)
              : led_off(cluster->indicator_led);

  hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_ON_OFF,
                                      ZCL_ATTR_ONOFF_INDICATOR_STATE);
}

void relay_cluster_on(zigbee_relay_cluster *cluster) {
  relay_on(cluster->relay);
  sync_indicator_led(cluster);
}

void relay_cluster_off(zigbee_relay_cluster *cluster) {
  relay_off(cluster->relay);
  sync_indicator_led(cluster);
}

void relay_cluster_toggle(zigbee_relay_cluster *cluster) {
  relay_toggle(cluster->relay);
  sync_indicator_led(cluster);
}

void relay_cluster_on_relay_change(zigbee_relay_cluster *cluster,
                                   uint8_t state) {
  hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_ON_OFF,
                                      ZCL_ATTR_ONOFF);
  if (cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TOGGLE ||
      cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TO_PREVIOUS) {
    relay_cluster_store_attrs_to_nv(cluster);
  }
}

void relay_cluster_on_write_attr(zigbee_relay_cluster *cluster,
                                 uint16_t attribute_id) {
  if (attribute_id == ZCL_ATTR_ONOFF_INDICATOR_STATE) {
    if (cluster->indicator_led->on) {
      led_on(cluster->indicator_led);
    } else {
      led_off(cluster->indicator_led);
    }
  }
  if (cluster->indicator_led_mode != ZCL_ONOFF_INDICATOR_MODE_MANUAL) {
    sync_indicator_led(cluster);
  }

  relay_cluster_store_attrs_to_nv(cluster);
}

typedef struct {
  uint8_t on_off;
  uint8_t startup_mode;
  uint8_t indicator_led_mode;
  uint8_t indicator_led_on;
} zigbee_relay_cluster_config;

static zigbee_relay_cluster_config nv_config_buffer;

void relay_cluster_store_attrs_to_nv(zigbee_relay_cluster *cluster) {
  nv_config_buffer.on_off = cluster->relay->on;
  nv_config_buffer.startup_mode = cluster->startup_mode;
  nv_config_buffer.indicator_led_mode = cluster->indicator_led_mode;
  if (cluster->indicator_led != NULL) {
    nv_config_buffer.indicator_led_on = cluster->indicator_led->on;
  }

  hal_nvm_write(NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx),
                sizeof(zigbee_relay_cluster_config),
                (uint8_t *)&nv_config_buffer);
}

void relay_cluster_load_attrs_from_nv(zigbee_relay_cluster *cluster) {
  hal_nvm_status_t st = hal_nvm_read(
      NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx),
      sizeof(zigbee_relay_cluster_config), (uint8_t *)&nv_config_buffer);

  if (st != HAL_NVM_SUCCESS)
    return;

  cluster->startup_mode = nv_config_buffer.startup_mode;
  cluster->indicator_led_mode = nv_config_buffer.indicator_led_mode;
}

void relay_cluster_handle_startup_mode(zigbee_relay_cluster *cluster) {
  hal_nvm_status_t st = hal_nvm_read(
      NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx),
      sizeof(zigbee_relay_cluster_config), (uint8_t *)&nv_config_buffer);

  if (st != HAL_NVM_SUCCESS)
    return;

  uint8_t prev_on = nv_config_buffer.on_off;

  switch (cluster->startup_mode) {
  case ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF:
    relay_cluster_off(cluster);
    break;

  case ZCL_START_UP_ONOFF_SET_ONOFF_TO_ON:
    relay_cluster_on(cluster);
    break;

  case ZCL_START_UP_ONOFF_SET_ONOFF_TOGGLE:
    if (prev_on) {
      relay_cluster_off(cluster);
    } else {
      relay_cluster_on(cluster);
    }
    break;

  case ZCL_START_UP_ONOFF_SET_ONOFF_TO_PREVIOUS:
    if (prev_on) {
      relay_cluster_on(cluster);
    } else {
      relay_cluster_off(cluster);
    }
    break;
  }

  // Restore indicator led
  if (cluster->indicator_led == NULL) {
    return;
  }

  if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_MANUAL) {
    nv_config_buffer.indicator_led_on ? led_on(cluster->indicator_led)
                                      : led_off(cluster->indicator_led);
  }
}
