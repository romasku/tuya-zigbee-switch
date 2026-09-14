#include "device_config/config_parser.h"
#include "device_config/device_type.h"
#include "device_config/nvm_items.h"
#include "device_config/reset.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/system.h"
#include "hal/timer.h"
#include "hal/zigbee.h"
#include "hal/zigbee_ota.h"
#include "zigbee/battery_cluster.h"
#include "zigbee/general_commands.h"
#ifdef END_DEVICE
#include "zigbee/poll_control_cluster.h"
#endif

void process_device_type_change() {
    // If device was updated from router to end device or vice versa,
    // we need to do a reset, as the network settings stored by SDK in NVM
    // are not compatible between these device types.
    // Read device type from NVM and compare with current configuration.
    enum device_type_t stored_device_type;
    hal_nvm_status_t   st =
        hal_nvm_read(NV_ITEM_DEVICE_TYPE, sizeof(stored_device_type),
                     (uint8_t *)&stored_device_type);

    if (st != HAL_NVM_SUCCESS) {
        // Unable to read device type from NVM, possibly first boot.
        stored_device_type = CURRENT_DEVICE_TYPE;
        hal_nvm_write(NV_ITEM_DEVICE_TYPE, sizeof(stored_device_type),
                      (uint8_t *)&stored_device_type);
        return;
    }
    if (stored_device_type != CURRENT_DEVICE_TYPE) {
        printf("Device type change detected: %d -> %d\r\n", stored_device_type,
               CURRENT_DEVICE_TYPE);
        // Device type has changed, update NVM and reset device.
        stored_device_type = CURRENT_DEVICE_TYPE;
        hal_nvm_write(NV_ITEM_DEVICE_TYPE, sizeof(stored_device_type),
                      (uint8_t *)&stored_device_type);
        // Perform a factory reset to clear incompatible network settings.
        hal_factory_reset();
        schedule_reboot(2000);
    }
}

void app_init(void) {
    handle_version_changes();
    parse_config(); // Does most of the setup, including all callbacks
                    // registration
    hal_zigbee_init_ota();
    init_global_attr_write_callback();

    process_device_type_change();
}

static bool boot_announce_sent = false;

// Retry steering with exponential backoff instead of on every main-loop
// pass: back-to-back channel scans keep the radio busy ~100% of the time,
// which no-neutral (parasitic) power supplies cannot sustain.
#define STEERING_BACKOFF_MIN_MS    (5 * 1000)
#define STEERING_BACKOFF_MAX_MS    (60 * 1000)

void app_task() {
    static uint32_t next_steering_attempt_ms = 0;
    static uint32_t steering_backoff_ms      = STEERING_BACKOFF_MIN_MS;

#ifdef END_DEVICE
    poll_control_cluster_update();
#endif

    hal_zigbee_network_status_t net_status = hal_zigbee_get_network_status();

    if (net_status == HAL_ZIGBEE_NETWORK_NOT_JOINED) {
        uint32_t now = hal_millis();
        if ((int32_t)(now - next_steering_attempt_ms) >= 0) {
            hal_zigbee_start_network_steering();
            next_steering_attempt_ms = now + steering_backoff_ms;
            if (steering_backoff_ms < STEERING_BACKOFF_MAX_MS) {
                steering_backoff_ms *= 2;
            }
        }
    } else if (net_status == HAL_ZIGBEE_NETWORK_JOINED) {
        next_steering_attempt_ms = 0;
        steering_backoff_ms      = STEERING_BACKOFF_MIN_MS;
        if (!boot_announce_sent) {
            // Only mark as sent when the announce actually went out; the
            // first send often races the freshly established parent link.
            boot_announce_sent =
                hal_zigbee_send_announce() == HAL_ZIGBEE_OK;
        }
    }
}
