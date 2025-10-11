#ifndef _RELAY_H_
#define _RELAY_H_

#include "types.h"

typedef void (*ev_relay_callback_t)(void *, u8);
typedef void (*poll_relay_callback_t)(void *);


typedef struct
{
  u32                   pin;
  u32                   off_pin;
  u8                    on_high;
  u8                    on;
  ev_relay_callback_t   on_change;
  void *                callback_param;
  poll_relay_callback_t poll_callback;
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
 * @return     none
 */
void relay_on(relay_t *relay);

/**
 * @brief      Disable the relay
 * @param	   *relay - Relay to use
 * @return     none
 */
void relay_off(relay_t *relay);

/**
 * @brief      Close the relay
 * @param	   *relay - Relay to use
 * @return     none
 */
void relay_toggle(relay_t *relay);

void relay_poll(relay_t *relay);

#endif
