#include "hal/timer.h"
#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)
#include <stdint.h>

static uint32_t s_millis_last_lo   = 0;
static uint32_t s_millis_overflows = 0;

uint32_t hal_millis() {
    // Convert system ticks directly to milliseconds.
    // clock_time() returns ticks at 16MHz, so divide by 16000 for milliseconds.
    // NOTE: clock_time() is a 32-bit register and overflows every ~268s.
    uint32_t now_lo = clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS;

    // Detect wraparound of the underlying 32-bit millisecond counter.
    // On a fresh boot, the very first hal_millis() call must not be flagged
    // as a wraparound (last_lo starts at 0 and now_lo grows from 0).
    if (s_millis_last_lo != 0 && now_lo < s_millis_last_lo) {
        s_millis_overflows++;
    }
    s_millis_last_lo = now_lo;
    return now_lo;
}

uint64_t hal_millis_64(void) {
    // Refresh the overflow counter first, then assemble the 64-bit value.
    hal_millis();
    return (((uint64_t)s_millis_overflows) << 32) | (uint64_t)s_millis_last_lo;
}
