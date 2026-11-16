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
 * Platforms with a 64-bit monotonic millisecond counter. The countdown
 * timer needs one to compare a deadline against the current time.
 *
 * End device builds are excluded: they sleep between polls, and the
 * system clock stops while they do, so a deadline could never be
 * reached on time.
 */
#if !defined(END_DEVICE) && (defined(HAL_TELINK) || defined(HAL_STUB))
#define HAL_HAS_TIMER_64    1
#endif

#ifdef HAL_HAS_TIMER_64

/**
 * Monotonic 64-bit millisecond counter. Safe for any deadline comparison.
 *
 * The counter is derived from the raw hardware tick, so it advances by
 * exactly one millisecond per millisecond of real time across the
 * hardware counter's ~268 s wrap.
 */
uint64_t hal_millis_64(void);

#endif

#endif
