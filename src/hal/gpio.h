#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include <stdint.h>

#define HAL_INVALID_PIN    0xFFFF

typedef uint16_t hal_gpio_pin_t;

typedef enum {
    HAL_GPIO_PULL_NONE    = 0,
    HAL_GPIO_PULL_UP      = 1,
    HAL_GPIO_PULL_UP_1M   = 2,
    HAL_GPIO_PULL_DOWN    = 3,
    HAL_GPIO_PULL_INVALID = 0xFF,
} hal_gpio_pull_t;

/**
 * Initialize GPIO pin as input/output with pull resistor configuration
 * @param gpio_pin GPIO pin identifier
 * @param is_input 1 for input, 0 for output
 * @param pull Pull resistor configuration
 */
void hal_gpio_init(hal_gpio_pin_t gpio_pin, uint8_t is_input,
                   hal_gpio_pull_t pull);

/**
 * Set output pin high
 * @param gpio_pin GPIO pin identifier
 */
void hal_gpio_set(hal_gpio_pin_t gpio_pin);

/**
 * Set output pin low
 * @param gpio_pin GPIO pin identifier
 */
void hal_gpio_clear(hal_gpio_pin_t gpio_pin);

/**
 * Set output pin based on value (0=low, non-zero=high)
 * @param gpio_pin GPIO pin identifier
 * @param value 0=low, non-zero=high
 */
static inline void hal_gpio_write(hal_gpio_pin_t gpio_pin, uint8_t value) {
    if (value) {
        hal_gpio_set(gpio_pin);
    } else {
        hal_gpio_clear(gpio_pin);
    }
}

/**
 * Read input pin state (0=low, 1=high)
 * @param gpio_pin GPIO pin identifier
 * @return Pin state (0=low, 1=high)
 */
uint8_t hal_gpio_read(hal_gpio_pin_t gpio_pin);

/**
 * Callback function type for GPIO state changes
 * Note: Called in task context, not interrupt routine to minimize race
 * conditions
 * @param gpio_pin GPIO pin that changed
 * @param arg User-provided argument
 */
typedef void (*gpio_callback_t)(hal_gpio_pin_t gpio_pin, void *arg);

/**
 * Register callback for pin state changes (enables interrupts and wake-up)
 * @param gpio_pin GPIO pin identifier
 * @param callback Function to call on state change
 * @param arg User argument passed to callback
 */
void hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback,
                       void *arg);

/**
 * Unregister pin callback (disables interrupts)
 * @param gpio_pin GPIO pin identifier
 */
void hal_gpio_unreg_callback(hal_gpio_pin_t gpio_pin);

/**
 * Parse pin string ("A5", "B10") to pin identifier
 * @param s Pin string (e.g., "A5", "B10")
 * @return Pin identifier or HAL_INVALID_PIN
 */
hal_gpio_pin_t hal_gpio_parse_pin(const char *s);

/**
 * Parse pull resistor string ("u"/"d"/"f" for up/down/float)
 * @param pull_str Pull string ("u"/"d"/"f")
 * @return Pull configuration or HAL_GPIO_PULL_INVALID
 */
hal_gpio_pull_t hal_gpio_parse_pull(const char *pull_str);

/*
 * Hardware GPIO Pulse Counter API
 *
 * Some platforms support hardware-based pulse counting on GPIO pins,
 * allowing accurate counting without CPU intervention.
 */

#define HAL_GPIO_COUNTER_INVALID    -1

typedef int8_t hal_gpio_counter_t;

typedef enum {
    HAL_GPIO_COUNTER_RISING  = 0,
    HAL_GPIO_COUNTER_FALLING = 1,
} hal_gpio_counter_edge_t;

/**
 * Initialize a hardware pulse counter on a GPIO pin
 * @param gpio_pin GPIO pin to count pulses on
 * @param edge Edge polarity to trigger counting (rising or falling)
 * @param pull Pull resistor configuration for the pin
 * @return Counter handle (>=0) on success, HAL_GPIO_COUNTER_INVALID on failure
 */
hal_gpio_counter_t hal_gpio_counter_init(hal_gpio_pin_t gpio_pin,
                                         hal_gpio_counter_edge_t edge,
                                         hal_gpio_pull_t pull);

/**
 * Deinitialize a hardware pulse counter and free resources
 * @param counter Counter handle from hal_gpio_counter_init
 */
void hal_gpio_counter_deinit(hal_gpio_counter_t counter);

/**
 * Read current pulse count from hardware counter
 * @param counter Counter handle from hal_gpio_counter_init
 * @return Current pulse count
 */
uint32_t hal_gpio_counter_read(hal_gpio_counter_t counter);

/**
 * Reset hardware counter to zero
 * @param counter Counter handle from hal_gpio_counter_init
 */
void hal_gpio_counter_reset(hal_gpio_counter_t counter);

/**
 * Start an already allocated hardware counter
 * @param counter Counter handle from hal_gpio_counter_init
 */
void hal_gpio_counter_start(hal_gpio_counter_t counter);

/**
 * Stop hardware counter but leaves the resource allocated
 * @param counter Counter handle from hal_gpio_counter_init
 */
void hal_gpio_counter_stop(hal_gpio_counter_t counter);

/**
 * Read current pulse count and reset the counter to zero
 * Note: Counter is stopped during read/reset, pulses during this brief
 * window may be lost. Counter is always restarted after this call.
 * @param counter Counter handle from hal_gpio_counter_init
 * @return Current pulse count before reset
 */
static inline uint32_t hal_gpio_counter_read_and_reset(hal_gpio_counter_t counter) {
    hal_gpio_counter_stop(counter);
    uint32_t count = hal_gpio_counter_read(counter);
    hal_gpio_counter_reset(counter);
    hal_gpio_counter_start(counter);
    return count;
}

#endif
