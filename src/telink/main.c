#include "ota_reformating/ensure_ota_scheme.h"
#include "stdint.h"
#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"

#include "telink_size_t_hack.h"

#include "app.h"
#include "hal/gpio.h"
#include "hal/tasks.h"
#include "hal/telink_zigbee_hal.h"
#include "hal/zigbee.h"

int real_main(startup_state_e state);

static _attribute_ram_code_sec_ bool is_bootloader_mode(void) {
  // Check if we are in bootloader mode by reading the flag
  return (*((u32 *)(BOOTLOADER_MODE_MAIN_ADDR + FLASH_TLNK_FLAG_OFFSET)) ==
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
  printf("Started!\r\n");

  uint8_t isRetention = (state == SYSTEM_DEEP_RETENTION) ? 1 : 0;

  os_init(isRetention);

  irq_enable();

  app_init();

  while (1) {
    ev_main();
    tl_zbTaskProcedure();
    app_task();
    report_handler();

#if PM_ENABLE
    if (!tl_stackBusy() && zb_isTaskDone()) {
      telink_gpio_hal_setup_wake_ups();
      ev_timer_event_t *timerEvt = ev_timer_nearestGet();
      u32 sleepDuration = 1000;
      if (timerEvt) {
        sleepDuration = timerEvt->timeout;
      }
      drv_pm_sleep(PM_SLEEP_MODE_SUSPEND,
                   PM_WAKEUP_SRC_PAD | PM_WAKEUP_SRC_TIMER, sleepDuration);
    }
#endif
  }

  return 0;
}