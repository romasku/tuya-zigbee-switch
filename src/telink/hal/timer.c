#include "hal/timer.h"
#include "tl_common.h"
#include <stdint.h>

uint32_t hal_millis() {
  // Convert system ticks directly to milliseconds
  // clock_time() returns ticks at 16MHz, so divide by 16000 for milliseconds
  return clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS;
}