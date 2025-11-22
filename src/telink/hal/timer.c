#include "hal/timer.h"
#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)
#include <stdint.h>

uint32_t hal_millis() {
  // Convert system ticks directly to milliseconds
  // clock_time() returns ticks at 16MHz, so divide by 16000 for milliseconds
  return clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS;
}