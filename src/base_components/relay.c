#include "relay.h"
#include "hal/printf_selector.h"
#include <stddef.h>
#include <stdbool.h>

void relay_init(relay_t *relay) { relay_off(relay, false); }

void relay_on(relay_t *relay, bool detached) {
  printf("relay_on\r\n");
  if (!detached) {
    hal_gpio_write(relay->pin, relay->on_high);
    if (relay->off_pin) {
      hal_gpio_write(relay->off_pin, !relay->on_high);
    }
  }
  relay->on = 1;
  if (relay->on_change != NULL) {
    relay->on_change(relay->callback_param, 1);
  }
}

void relay_off(relay_t *relay, bool detached) {
  printf("relay_off\r\n");
  if (!detached) {
    hal_gpio_write(relay->pin, !relay->on_high);
    if (relay->off_pin) {
      hal_gpio_write(relay->off_pin, relay->on_high);
    }
  }
  relay->on = 0;
  if (relay->on_change != NULL) {
    relay->on_change(relay->callback_param, 0);
  }
}

void relay_toggle(relay_t *relay, bool detached) {
  printf("relay_toggle\r\n");
  if (relay->on) {
    relay_off(relay, detached);
  } else {
    relay_on(relay, detached);
  }
}
