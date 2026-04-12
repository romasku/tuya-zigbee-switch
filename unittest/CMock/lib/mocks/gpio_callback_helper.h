#ifndef _GPIO_CALLBACK_HELPER_H_
#define _GPIO_CALLBACK_HELPER_H_

#include "Mockgpio.h"

void trigger_pin_change(int cb_cnt);

void captured_hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback, void *arg, int cmock_num_calls);

#endif