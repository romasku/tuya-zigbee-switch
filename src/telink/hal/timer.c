#include "hal/timer.h"
#include "hal/tick64.h"
#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)
#include <stdint.h>

// clock_time() is a 32-bit counter at 16 MHz: it wraps every 268.435456 s.
// hal_tick64_millis() counts the elapsed tick delta, so the value stays
// monotonic across that wrap.
static hal_tick64_state s_tick_state;

uint32_t hal_millis() {
    // Low 32 bits of the true millisecond timeline, so it wraps every
    // ~49.7 days and "now - then" differences stay correct.
    return (uint32_t)hal_tick64_millis(&s_tick_state, clock_time());
}

uint64_t hal_millis_64(void) {
    // Monotonic across the hardware tick wrap. Deadlines of any length
    // compare correctly (the ZCL On With Timed Off maximum is 6553.4 s).
    return hal_tick64_millis(&s_tick_state, clock_time());
}
