#include "device_config/config_parser.h"
#include "hal/zigbee.h"
#include "hal/zigbee_ota.h"
#include "zigbee/general_commands.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/consts.h"

// Forward declarations from config_parser.c
extern zigbee_relay_cluster relay_clusters[];
extern uint8_t relay_clusters_cnt;

void app_init(void) {
  handle_version_changes();
  parse_config(); // Does most of the setup, including all callbacks
                  // registration
  hal_zigbee_init_ota();
  init_global_attr_write_callback();
}

static bool boot_announce_sent = false;

void app_task() {
  // TODO: add jitter to avoid all devices trying to join at once
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED &&
      hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINING) {
    hal_zigbee_start_network_steering();
  }
  if (!boot_announce_sent &&
      hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED) {
    hal_zigbee_send_announce();
    // Report all relay states on boot so Z2M knows the current state
    for (uint8_t i = 0; i < relay_clusters_cnt; i++) {
      hal_zigbee_notify_attribute_changed(relay_clusters[i].endpoint,
                                          ZCL_CLUSTER_ON_OFF,
                                          ZCL_ATTR_ONOFF);
    }
    boot_announce_sent = true;
  }
}