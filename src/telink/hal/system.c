#include "hal/system.h"
#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#pragma pack(pop)
#include <stdbool.h>
#include <stdint.h>

void hal_system_reset(void) {
  // Telink 8258 system reset
  mcu_reset();
}

void hal_factory_reset(void) { zb_factoryReset(); }