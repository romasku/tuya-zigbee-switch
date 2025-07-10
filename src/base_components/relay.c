#include "relay.h"
#include "tl_common.h"
#include "millis.h"


void relay_init(relay_t *relay)
{
  relay_off(relay);
}

void relay_on(relay_t *relay)
{
  printf("relay_on\r\n");

  if (relay->off_pin) {
    // Bi-stable Latch relay: Pulse ON pin for 125ms
    drv_gpio_write(relay->pin, relay->on_high);
    WaitMs(125);
    drv_gpio_write(relay->pin, !relay->on_high);
  } else {
    // Normal relay: Turn on the active pin
    drv_gpio_write(relay->pin, relay->on_high);
  }
  relay->on = 1;
  if (relay->on_change != NULL)
  {
    relay->on_change(relay->callback_param, 1);
  }
}

void relay_off(relay_t *relay)
{
  printf("relay_off\r\n");

  if (relay->off_pin) {
    // Bi-stable Latch relay: Pulse OFF pin for 125ms
    drv_gpio_write(relay->off_pin, relay->on_high);
    WaitMs(125);
    drv_gpio_write(relay->off_pin, !relay->on_high);
  } else {
    // Normal relay: Turn off the active pin
    drv_gpio_write(relay->pin, !relay->on_high);
  }
  relay->on = 0;
  if (relay->on_change != NULL)
  {
    relay->on_change(relay->callback_param, 0);
  }
}

void relay_toggle(relay_t *relay)
{
  printf("relay_toggle\r\n");
  if (relay->on)
  {
    relay_off(relay);
  }
  else
  {
    relay_on(relay);
  }
}
