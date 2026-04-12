#include "encoder.h"
#include "hal/printf_selector.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include <stdbool.h>
#include <stddef.h>

void _encoder_gpio_callback(hal_gpio_pin_t pin, void *arg);
void _pinAChanged(uint8_t new_state, encoder_t *encoder);

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
}

void _encoder_gpio_callback(hal_gpio_pin_t pin, void *arg)
{
  encoder_t *encoder = (encoder_t *)arg;
  uint8_t new_state = hal_gpio_read(pin);

  if (pin == encoder->pin_a && new_state != encoder->pin_a_state && (hal_millis() - encoder->pin_a_last_change) > 50)
  {
    // Pin A Changed and has not changed in the last 50 milliseconds
    printf("Pin A (%d) Changed!\r\n", encoder->pin_a);

    encoder->pin_a_state = new_state;
    encoder->pin_a_last_change = hal_millis();

    _pinAChanged(new_state, encoder);
  }

  if (pin == encoder->pin_b && new_state != encoder->pin_b_state && (hal_millis() - encoder->pin_b_last_change) > 50)
  {
    // Pin B Changed and has not changed in the last 50 milliseconds
    printf("Pin B (%d) Changed!\r\n", encoder->pin_b);

    encoder->pin_b_state = new_state;
    encoder->pin_b_last_change = hal_millis();
  }

  if (pin == encoder->pin_sw && new_state != encoder->pin_sw_state && (hal_millis() - encoder->pin_sw_last_change) > 50)
  {
    // Pin SW Changed and has not changed in the last 50 milliseconds
    printf("Pin SW Changed!\r\n");

    if (encoder->on_press != NULL)
    {
      encoder->on_press();
    }

    encoder->pin_sw_state = new_state;
    encoder->pin_sw_last_change = hal_millis();
  }
}

void _pinAChanged(uint8_t new_state, encoder_t *encoder)
{
  if (new_state != encoder->pin_b)
  {
    printf("Rotating CCW\r\n");

    if (encoder->on_rotate_ccw != NULL)
    {
      encoder->on_rotate_ccw();
    }
  }
}