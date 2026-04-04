#include "ota_reformating/ensure_ota_scheme.h"
#include "stdint.h"

#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"
#pragma pack(pop)

#include "telink_size_t_hack.h"

#include "device_config/config_parser.h"

#include "app.h"
#include "hal/gpio.h"
#include "hal/telink_zigbee_hal.h"
#include "hal/zigbee.h"

int real_main(startup_state_e state);

static _attribute_ram_code_sec_ bool is_bootloader_mode(void) {
    // Check if we are in bootloader mode by reading the flag
    return(*((u32 *)(BOOTLOADER_MODE_MAIN_ADDR + FLASH_TLNK_FLAG_OFFSET)) ==
           TL_START_UP_FLAG_WHOLE);
}

_attribute_ram_code_sec_ int main(void) {
    if (is_bootloader_mode()) {
        // In bootloader mode, system is partally initialized by bootloader,
        // so no need to call drv_platform_init here.
        // BUT! We cannot call any flash-resident code, as it was linked to run from
        // different offset. So only ram-code functions are allowed here.
        // For example, DO NOT use printf here!
        ensure_correct_ota_scheme();
        SYSTEM_RESET(); // Should not return from above, but just in case, reset
    }

    startup_state_e state = drv_platform_init();
    // Ensure we are not in small-OTA mode.
    ensure_correct_ota_scheme();

    return real_main(state);
}

int real_main(startup_state_e state) {
    uint8_t isRetention = (state == SYSTEM_DEEP_RETENTION) ? 1 : 0;

    os_init(isRetention);

    irq_enable();

    if (!isRetention) {
        app_init();
    }
#ifdef ZB_ED_ROLE
    else {
        // Re-configure radio PHY — hardware registers are lost during deep
        // retention.  Without this the MAC layer can hang on the next Data
        // Request (e.g. tl_zbNwkQuickDataPollCb), keeping the radio powered
        // (~4 mA) indefinitely.  Matches the Telink SDK pattern used in
        // sampleContactSensor / sampleSwitch.
        mac_phyReconfig();

        telink_gpio_reinit_after_deep_retention();
        telink_gpio_reinit_interrupts();
    }
#endif

    if (battery.pin != HAL_INVALID_PIN) {
        // Use lower TX power if battery powered
        g_zb_txPowerSet = RF_POWER_INDEX_P3p01dBm;
    }

    drv_wd_setInterval(1000);
    drv_wd_start();

    while (1) {
        drv_wd_clear();
        ev_main();
        drv_wd_clear();
        tl_zbTaskProcedure();
        drv_wd_clear();
        app_task();
        drv_wd_clear();
        if (battery.pin == HAL_INVALID_PIN) {
            report_handler();
        }
        drv_wd_clear();

#if PM_ENABLE
        if (!tl_stackBusy() && zb_isTaskDone()) {
            telink_gpio_hal_setup_wake_ups();
            // Only use deep retention for battery devices,
            // as it messes with GPIO output state, and relays cannot be
            // driven via PULL-ups, it may cause issues.
            if (battery.pin != HAL_INVALID_PIN) {
                telink_gpio_to_pull_for_deep_retention();
                drv_pm_lowPowerEnter();
                // If we didn't actually enter deep retention, restore GPIO
                // as it was configured to use pulls for retention
                telink_gpio_reinit_after_deep_retention();
            } else {
                ev_timer_event_t *timerEvt = ev_timer_nearestGet();
                u32 sleepDuration          = 1000;
                if (timerEvt) {
                    sleepDuration = timerEvt->timeout < 1000 ? timerEvt->timeout : 1000;
                }
                drv_pm_sleep(PM_SLEEP_MODE_SUSPEND,
                             PM_WAKEUP_SRC_PAD | PM_WAKEUP_SRC_TIMER, sleepDuration);
            }
        }
#endif
    }

    return 0;
}
