#ifndef _NETWORK_INDICATOR_H_
#define _NETWORK_INDICATOR_H_

#include "led.h"
#include <stdbool.h>

typedef struct {
  led_t *leds[4];
  bool has_dedicated_led;
  bool manual_state_when_connected;
} network_indicator_t;

void network_indicator_connected(network_indicator_t *indicator);

void network_indicator_from_manual_state(network_indicator_t *indicator);

void network_indicator_commission_success(network_indicator_t *indicator);

void network_indicator_not_connected(network_indicator_t *indicator);

#endif
