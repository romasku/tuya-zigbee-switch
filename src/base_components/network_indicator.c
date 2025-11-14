#include "network_indicator.h"
#include <stddef.h>

void network_indicator_connected(network_indicator_t *indicator) {
  network_indicator_from_manual_state(indicator);
}

void network_indicator_from_manual_state(network_indicator_t *indicator) {
  led_t **led = indicator->leds;

  while (*led != NULL && (led - indicator->leds) < 4) {
    (*led)->blink_times_left = 0;
    if (indicator->has_dedicated_led) {
      if (indicator->manual_state_when_connected) {
        led_on(*led);
      } else {
        led_off(*led);
      }
    }
    led++;
  }
}

void network_indicator_commission_success(network_indicator_t *indicator) {
  led_t **led = indicator->leds;

  while (*led != NULL && (led - indicator->leds) < 4) {
    led_blink(*led, 500, 500, 7);
    led++;
  }
}

void network_indicator_not_connected(network_indicator_t *indicator) {
  led_t **led = indicator->leds;

  while (*led != NULL && (led - indicator->leds) < 4) {
    if ((*led)->blink_times_left != LED_BLINK_FOREVER) {
      led_blink(*led, 500, 500, LED_BLINK_FOREVER);
    }
    led++;
  }
}
