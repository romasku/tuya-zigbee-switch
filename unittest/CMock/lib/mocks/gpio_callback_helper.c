#include "Mockgpio.h"
#include "gpio_callback_helper.h"

gpio_callback_t gpio_callbacks[3];
hal_gpio_pin_t gpio_pins[3];
void *pin_args[3];

void trigger_pin_change()
{
  gpio_callbacks[0](gpio_pins[0], pin_args[0]);
}

void captured_hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback, void *arg, int cmock_num_calls)
{
  gpio_pins[cmock_num_calls] = gpio_pin;
  gpio_callbacks[cmock_num_calls] = callback;
  pin_args[cmock_num_calls] = arg;
}