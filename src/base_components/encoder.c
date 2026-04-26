#include "encoder.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include <stdbool.h>
#include <stddef.h>

void _encoder_gpio_callback(hal_gpio_pin_t pin, void *arg);
void _pinAChanged(uint8_t new_state, encoder_t *encoder);
void _pinBChanged(uint8_t new_state, encoder_t *encoder);
void _pinSWChanged(uint8_t new_state, encoder_t *encoder);

void encoder_init(encoder_t *encoder)
{
  printf("Encoder Init, with PinA: %d, PinB: %d, PinSW: %d\r\n", encoder->pin_a, encoder->pin_b, encoder->pin_sw);

  encoder->pin_a_state = hal_gpio_read(encoder->pin_a);
  encoder->pin_a_last_change = hal_millis();
  encoder->pin_b_state = hal_gpio_read(encoder->pin_b);
  encoder->pin_b_last_change = hal_millis();
  encoder->pin_sw_state = hal_gpio_read(encoder->pin_sw);
  encoder->pin_sw_last_change = hal_millis();

  hal_gpio_callback(encoder->pin_a, _encoder_gpio_callback, encoder);
  hal_gpio_callback(encoder->pin_b, _encoder_gpio_callback, encoder);
  hal_gpio_callback(encoder->pin_sw, _encoder_gpio_callback, encoder);

  encoder->rotate_since_pressed = false;
}

void _encoder_gpio_callback(hal_gpio_pin_t pin, void *arg)
{
  encoder_t *encoder = (encoder_t *)arg;
  uint8_t new_state = hal_gpio_read(pin);
  uint32_t now = hal_millis();

  if (pin == encoder->pin_a && new_state != encoder->pin_a_state && (now - encoder->pin_a_last_change) > 10)
  {
    // Pin A Changed and has not changed in the last 50 milliseconds
    encoder->pin_a_state = new_state;
    encoder->pin_a_last_change = now;

    _pinAChanged(new_state, encoder);
  }

  if (pin == encoder->pin_b && new_state != encoder->pin_b_state && (now - encoder->pin_b_last_change) > 10)
  {
    // Pin B Changed and has not changed in the last 50 milliseconds
    encoder->pin_b_state = new_state;
    encoder->pin_b_last_change = now;

    _pinBChanged(new_state, encoder);
  }

  if (pin == encoder->pin_sw && new_state != encoder->pin_sw_state && (now - encoder->pin_sw_last_change) > 10)
  {
    encoder->pin_sw_state = new_state;
    encoder->pin_sw_last_change = now;

    _pinSWChanged(new_state, encoder);
  }
}

void _pinAChanged(uint8_t new_state, encoder_t *encoder)
{
  if (new_state != encoder->pin_b_state)
  {
    encoder->rotate_since_pressed = true;

    if (encoder->pin_sw_state == 0)
    {
      printf("Rotating CCW while Pressed\r\n");
      if (encoder->on_rotate_ccw_while_pressed != NULL)
      {
        encoder->on_rotate_ccw_while_pressed(encoder->callback_param);
      }
    }
    else
    {
      printf("Rotating CCW\r\n");
      if (encoder->on_rotate_ccw != NULL)
      {
        encoder->on_rotate_ccw(encoder->callback_param);
      }
    }
  }
}

void _pinBChanged(uint8_t new_state, encoder_t *encoder)
{
  if (new_state != encoder->pin_a_state)
  {
    encoder->rotate_since_pressed = true;

    if (encoder->pin_sw_state == 0)
    {
      printf("Rotating CW while Pressed\r\n");
      if (encoder->on_rotate_cw_while_pressed != NULL)
      {
        encoder->on_rotate_cw_while_pressed(encoder->callback_param);
      }
    }
    else
    {
      printf("Rotating CW\r\n");

      if (encoder->on_rotate_cw != NULL)
      {
        encoder->on_rotate_cw(encoder->callback_param);
      }
    }
  }
}

void _pinSWChanged(uint8_t new_state, encoder_t *encoder)
{
  if (new_state == 0)
  {
    printf("Pressed\r\n");

    encoder->rotate_since_pressed = false;
  }
  else
  {
    printf("Released\r\n");

    if (!encoder->rotate_since_pressed && encoder->on_press != NULL)
      encoder->on_press(encoder->callback_param);
  }
}