#ifndef _RELAY_H_
#define _RELAY_H_

#include "hal/gpio.h"
#include "hal/tasks.h"
#include <stdint.h>

typedef void (*relay_callback_t)(void *param, uint8_t state);

typedef struct {
  int pin;                    // ON pin
  int off_pin;                // OFF pin (optional, for latching relays)
  int on_high;                // 1 if "on" is HIGH, 0 if "on" is LOW
  int on;                     // Current state (0 = off, 1 = on)
  hal_task_t latching_task;   // Task to clear pulse for latching relays
  relay_callback_t on_change; // Optional callback for state change
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
