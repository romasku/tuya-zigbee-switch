#include "ota_reformating/ensure_ota_scheme.h"
#include "stdint.h"

#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"
#pragma pack(pop)

#include "telink_size_t_hack.h"

#include "app.h"
#include "hal/gpio.h"
#include "hal/tasks.h"
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

    if (isRetention) {
        printf("[0] Wakeup from retention\r\n");
    } else {
        printf("[0] Cold boot\r\n");
    }

    os_init(isRetention);

    irq_enable();

    if (!isRetention) {
    app_init();
    } else {
        // Re-configure radio PHY — hardware registers are lost during deep
        // retention.  Without this the MAC layer can hang on the next Data
        // Request (e.g. tl_zbNwkQuickDataPollCb), keeping the radio powered
        // (~4 mA) indefinitely.  Matches the Telink SDK pattern used in
        // sampleContactSensor / sampleSwitch.
        mac_phyReconfig();

        app_reinit_retention();
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
        report_handler();
        drv_wd_clear();

#if PM_ENABLE
        if (!tl_stackBusy() && zb_isTaskDone() && hal_zigbee_is_sleep_allowed()) {
            apsCleanToStopSecondClock();
            telink_gpio_hal_setup_wake_ups();

            u32 r = drv_disable_irq();

            ev_timer_event_t *timerEvt = ev_timer_nearestGet();
#if UART_PRINTF_MODE
            // Debug: use deep sleep like production, but log timers
            if (timerEvt) {
                printf("T %d cb=%x d=%x\r\n", timerEvt->timeout,
                       (u32)timerEvt->cb, (u32)timerEvt->data);
                // Give UART time to flush before deep sleep (~2ms for 48 chars @ 115200)
                sleep_us(3000);
            }
#endif
            // Always use deep retention for realistic power consumption
            u32 sleepDuration = PM_NORMAL_SLEEP_MAX;
            if (timerEvt && timerEvt->timeout < sleepDuration) {
                sleepDuration = timerEvt->timeout;
            }
#if UART_PRINTF_MODE
            printf("[%d] Entering deep sleep for %d ms\r\n", 
                   clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS, sleepDuration);
            sleep_us(3000); // Give UART time to flush
#endif
            rf_paShutDown();

            if (sleepDuration > PM_NORMAL_SLEEP_MAX) {
                drv_pm_longSleep(PM_SLEEP_MODE_DEEP_WITH_RETENTION,
                                 PM_WAKEUP_SRC_PAD | PM_WAKEUP_SRC_TIMER,
                                 sleepDuration);
            } else {
                drv_pm_sleep(PM_SLEEP_MODE_DEEP_WITH_RETENTION,
                             PM_WAKEUP_SRC_PAD | PM_WAKEUP_SRC_TIMER,
                             sleepDuration);
            }

            drv_restore_irq(r);
        }
#endif
    }

    return 0;
}
