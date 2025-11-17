#include "device_config/config_parser.h"
#include "hal/zigbee.h"
#include "hal/zigbee_ota.h"
#include "zigbee/general_commands.h"

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
    boot_announce_sent = true;
  }
}