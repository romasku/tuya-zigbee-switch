#include "device_config/config_parser.h"
#include "device_config/device_type.h"
#include "device_config/nvm_items.h"
#include "device_config/reset.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/system.h"
#include "hal/zigbee.h"
#include "hal/zigbee_ota.h"
#include "zigbee/general_commands.h"

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

void app_reinit_retention(void) {
    // After deep retention wake, re-init peripherals whose SFRs were lost.
    // All SRAM state (config, clusters, ZB stack) is preserved by os_init(1).

#ifdef BATTERY_POWERED
    // ADC must be re-initialised BEFORE config_reinit_gpio(), because
    // btn_retention_wake() fires press callbacks which read the battery.
    // Without this the retained battery_adc_initialized flag skips hw init
    // and the ADC returns 0 mV.
    hal_battery_reinit_after_retention();
#endif

    config_reinit_gpio();
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

void app_task() {
    // Check join settle timer (must be done here, not in sleep check)
    hal_zigbee_check_settle_timer();
    hal_zigbee_check_report_active_timer();
    
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
