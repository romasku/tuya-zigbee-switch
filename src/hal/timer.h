#ifndef _HAL_TIMER_H_
#define _HAL_TIMER_H_

#include <stdint.h>

/**
 * Get milliseconds elapsed since system initialization
 * @return Number of milliseconds since system started
 */
uint32_t hal_millis();

#endif
