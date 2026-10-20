#ifndef _HAL_TIMER_H_
#define _HAL_TIMER_H_

#include <stdint.h>

/**
 * Get milliseconds elapsed since system initialization
 * @return Number of milliseconds since system started
 *
 * NOTE: backed by a 32-bit hardware tick counter that wraps every
 * ~268 s. Use hal_millis_64() for comparisons that must survive the wrap.
 */
uint32_t hal_millis();

/**
 * Monotonic 64-bit millisecond counter. Safe for any deadline comparison.
 */
uint64_t hal_millis_64(void);

#endif
