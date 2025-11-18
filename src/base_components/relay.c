#include "relay.h"
#include "tl_common.h"
#include "millis.h"
#include "../zigbee/relay_cluster.h"


void relay_init(relay_t *relay)
{
  relay_off(relay);
}

void relay_on(relay_t *relay)
{
  printf("relay_on\r\n");
  drv_gpio_write(relay->pin, relay->on_high);
  if (relay->off_pin)
  {
    drv_gpio_write(relay->off_pin, !relay->on_high);
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
  drv_gpio_write(relay->pin, !relay->on_high);
  if (relay->off_pin)
  {
    drv_gpio_write(relay->off_pin, relay->on_high);
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

void relay_poll(relay_t *relay)
{
  if (relay->poll_callback)
    relay->poll_callback(relay->callback_param);
}