#ifndef _LED_H_
#define _LED_H_

#include "hal/gpio.h"
#include "hal/tasks.h"
#include <stdint.h>

typedef struct {
    hal_gpio_pin_t pin;
    uint8_t        on_high;
    uint8_t        on;
    uint16_t       blink_times_left;
    uint16_t       blink_time_on;
    uint16_t       blink_time_off;
    hal_task_t     blink_task;
} led_t;

/**
 * @brief      Initialize led (set initial state)
 * @param	   *led - Led to use
 * @return     none
 */
void led_init(led_t *led);

/**
 * @brief      Turn on led, canceling any blinking
 * @param	   *led - Led to use
 * @return     none
 */
void led_on(led_t *led);

/**
 * @brief      Turn off led, canceling any blinking
 * @param	   *led - Led to use
 * @return     none
 */
void led_off(led_t *led);

#define LED_BLINK_FOREVER    0xFFFF

/**
 * @brief      Start led blinking, will go to off when finished
 * @param	     *led - Led to use
 *             on_time_ms - Time led should be on in milliseconds
 *             off_time_ms - Time led should be off in milliseconds
 *             times - Times to repeat blink before returning to fixed state,
 *                     0xFFFF - blink forever
 * @return     none
 */
void led_blink(led_t *led, uint16_t on_time_ms, uint16_t off_time_ms,
               uint16_t times);

#endif
