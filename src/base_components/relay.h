#ifndef _RELAY_H_
#define _RELAY_H_

#include "hal/gpio.h"
#include <stdint.h>
#include <stdbool.h>

typedef void (*ev_relay_callback_t)(void *, uint8_t);

typedef struct {
  hal_gpio_pin_t pin;
  hal_gpio_pin_t off_pin;
  uint8_t on_high;
  uint8_t on;
  ev_relay_callback_t on_change;
  void *callback_param;
} relay_t;

/**
 * @brief      Initialize relay (set initial state)
 * @param	   *relay - Relay to use
 * @return     none
 */
void relay_init(relay_t *relay);

/**
 * @brief      Enable the relay
 * @param	   *relay - Relay to use
 * @param      detached - do not change physical state
 * @return     none
 */
void relay_on(relay_t *relay, bool detached);

/**
 * @brief      Disable the relay
 * @param	   *relay - Relay to use
 * @param      detached - do not change physical state
 * @return     none
 */
void relay_off(relay_t *relay, bool detached);

/**
 * @brief      Toggle the relay
 * @param	   *relay - Relay to use
 * @param      detached - do not change physical state
 * @return     none
 */
void relay_toggle(relay_t *relay, bool detached);

#endif
