#ifndef _RELAY_H_
#define _RELAY_H_

#include "hal/gpio.h"
#include "hal/tasks.h"
#include <stdint.h>

typedef void (*relay_callback_t)(void *param, uint8_t state);

// Backwards compatibility alias
typedef relay_callback_t ev_relay_callback_t;

typedef struct {
  hal_gpio_pin_t pin;
  uint8_t turn_off;
  hal_task_t task;
  uint8_t in_use;
} relay_pulse_t;

typedef struct {
    int pin;                    // ON pin
    int off_pin;                // OFF pin (optional, for bistable relays)
    int on_high;                // 1 if "on" is HIGH, 0 if "on" is LOW
    int on;                     // Current state (0 = off, 1 = on)
    hal_task_t clear_task;      // Task to clear pulse for bistable relays
    int clear_task_active;      // Flag to indicate if the clear task is active
    void (*on_change)(void *, int); // Optional callback for state change
    void *callback_param;       // Parameter passed to callback
} relay_t;

/**
 * @brief      Initialize relay (set initial state)
 * @param      *relay - Relay to use
 * @return     none
 */
void relay_init(relay_t *relay);

/**
 * @brief      Turn on relay
 * @param      *relay - Relay to use
 * @return     none
 */
void relay_on(relay_t *relay);

/**
 * @brief      Turn off relay
 * @param      *relay - Relay to use
 * @return     none
 */
void relay_off(relay_t *relay);

/**
 * @brief      Toggle relay state
 * @param      *relay - Relay to use
 * @return     none
 */
void relay_toggle(relay_t *relay);

#endif
