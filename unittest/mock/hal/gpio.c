#include "hal/gpio.h"
#include <stdio.h>

void hal_gpio_init(hal_gpio_pin_t gpio_pin, uint8_t is_input,
                   hal_gpio_pull_t pull)
{
  printf("hal_gpio_init called, not implemented\n\r");
}

void hal_gpio_set(hal_gpio_pin_t gpio_pin)
{
  printf("hal_gpio_set called, not implemented\n\r");
}

void hal_gpio_clear(hal_gpio_pin_t gpio_pin)
{
  printf("hal_gpio_set called, not implemented\n\r");
}

void hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback,
                       void *arg)
{
  printf("hal_gpio_callback called, not implemented\n\r");
}

void hal_gpio_unreg_callback(hal_gpio_pin_t gpio_pin)
{
  printf("hal_gpio_unreg_callback called, not implemented\n\r");
}

hal_gpio_pin_t hal_gpio_parse_pin(const char *s)
{
  printf("hal_gpio_parse_pin called, not implemented\n\r");
  return 1;
}

hal_gpio_pull_t hal_gpio_parse_pull(const char *pull_str)
{
  printf("hal_gpio_parse_pull called, not implemented\n\r");
  return 1;
}

uint8_t hal_gpio_read(hal_gpio_pin_t gpio_pin)
{
  printf("Mocking Pin Read as 1\n\r");
  return 1;
}