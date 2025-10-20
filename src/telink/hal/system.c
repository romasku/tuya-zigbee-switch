#include "hal/system.h"
#include "tl_common.h"
#include <stdbool.h>
#include <stdint.h>

void hal_system_reset(void) {
  // Telink 8258 system reset
  mcu_reset();
}