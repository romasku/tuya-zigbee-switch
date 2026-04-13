#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "hal/gpio.h"
#include "hal/tasks.h"
#include <stdint.h>

typedef void (*ev_encoder_callback_t)();

typedef struct
{
  hal_gpio_pin_t pin_a; // Also known as CLK
  uint8_t pin_a_state;
  uint32_t pin_a_last_change;

  hal_gpio_pin_t pin_b; // Also known as DT
  uint8_t pin_b_state;
  uint32_t pin_b_last_change;

  hal_gpio_pin_t pin_sw;
  uint8_t pin_sw_state;
  uint32_t pin_sw_last_change;

  ev_encoder_callback_t on_press;
  ev_encoder_callback_t on_rotate_ccw;
  ev_encoder_callback_t on_rotate_cw;
  ev_encoder_callback_t on_rotate_ccw_while_pressed;
  ev_encoder_callback_t on_rotate_cw_while_pressed;
  void *callback_param;
} encoder_t;

void encoder_init(encoder_t *encoder);

#endif