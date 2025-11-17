#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include <stdint.h>

#define HAL_INVALID_PIN 0xFFFF

typedef uint16_t hal_gpio_pin_t;

typedef enum {
  HAL_GPIO_PULL_NONE = 0,
  HAL_GPIO_PULL_UP = 1,
  HAL_GPIO_PULL_DOWN = 2,
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

#endif
