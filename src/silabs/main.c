#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif
#include "sl_system_init.h"
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "sl_system_kernel.h"
#else
#include "sl_system_process_action.h"
#endif // SL_CATALOG_KERNEL_PRESENT

#include "app/framework/include/af.h"
#include "em_gpio.h"

#include "app.h"
#include "base_components/button.h"
#include "base_components/led.h"
#include "base_components/network_indicator.h"
#include "base_components/relay.h"
#include "device_config/config_nv.h"
#include "device_config/config_parser.h"
#include "hal/gpio.h"
#include "hal/timer.h"
#include "hal/zigbee.h"
#include "zigbee/basic_cluster.h"
#include "zigbee/consts.h"
#include "zigbee/general_commands.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/switch_cluster.h"

// TODO: make configurable via ZCL
#define POLLING_INTERVAL_MS 100

int main(void) {
  // Initialize Silicon Labs device, system, service(s) and protocol stack(s).
  // Note that if the kernel is present, processing task(s) will be created by
  // this call.
  sl_system_init();

  app_init();

  // Switch should never "long poll", as it should always be somewhat reactive
  // to ZCL commands.
  sl_zigbee_af_set_short_poll_interval_ms_cb(POLLING_INTERVAL_MS);
  sl_zigbee_af_set_long_poll_interval_ms_cb(POLLING_INTERVAL_MS);

#if defined(SL_CATALOG_KERNEL_PRESENT)
  // Start the kernel. Task(s) created in app_init() will start running.
  sl_system_kernel_start();
#else // SL_CATALOG_KERNEL_PRESENT

  while (1) {
    // Do not remove this call: Silicon Labs components process action routine
    // must be called from the super loop.
    sl_system_process_action();

    // Application process.
    app_task();

    // Let the CPU go to sleep if the system allow it.
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    sl_power_manager_sleep();
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
  }
#endif // SL_CATALOG_KERNEL_PRESENT

  return 0;
}
