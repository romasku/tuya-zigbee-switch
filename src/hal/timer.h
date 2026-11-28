#ifndef _HAL_TIMER_H_
#define _HAL_TIMER_H_

#include <stdint.h>

/**
 * Get milliseconds elapsed since system initialization
 * @return Number of milliseconds since system started
 *
 * This is the low 32 bits of the true millisecond timeline, so it wraps
 * every 2^32 ms (~49.7 days) and "now - then" differences are correct
 * for any interval shorter than that.
 */
uint32_t hal_millis();

/**
 * Platforms with a millisecond clock that can carry a countdown deadline.
 *
 * The countdown compares its deadline against hal_millis() with a signed
 * 32-bit difference, so it needs a clock that only wraps every 2^32 ms
 * and a deadline shorter than 2^31 ms (~24.8 days). The ZCL maximum of
 * 6553 s is far below that.
 *
 * End device builds are included: they sleep between polls, but the SDK
 * restores the system tick after a suspend sleep, so the clock keeps
 * counting real time while the device sleeps. A countdown is then checked
 * once per wake-up, so it ends about a second after its deadline.
 */
#if defined(HAL_TELINK) || defined(HAL_STUB)
#define HAL_HAS_COUNTDOWN_TIMER    1
#endif

#endif
