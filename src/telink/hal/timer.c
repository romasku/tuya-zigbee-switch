#include "hal/timer.h"
#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)
#include <stdint.h>

// clock_time() is a 32-bit counter of 16 MHz ticks: it wraps every 268 seconds.
// Dividing it gave a millisecond value that jumped back to 0 at each wrap, which broke every
// "now - then" in the callers (false long presses, false multi presses, fast poll that never ends).
// Accumulate the elapsed ticks instead. Correct as long as this is called at least once per
// 268 seconds: app_task() calls it in every main loop round.
uint32_t hal_millis() {
    static uint32_t last_tick  = 0;
    static uint32_t rest_ticks = 0;
    static uint32_t millis     = 0;

    uint32_t now = clock_time();

    rest_ticks += now - last_tick; // < 2 * 2^32 / 16000 ticks can never overflow: rest_ticks < 16000
    last_tick   = now;
    millis     += rest_ticks / CLOCK_16M_SYS_TIMER_CLK_1MS;
    rest_ticks %= CLOCK_16M_SYS_TIMER_CLK_1MS;
    return millis;
}
