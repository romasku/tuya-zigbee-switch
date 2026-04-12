/* =========================================================================
    Unity - A Test Framework for C
    ThrowTheSwitch.org
    Copyright (c) 2007-26 Mike Karlesky, Mark VanderVoord, & Greg Williams
    SPDX-License-Identifier: MIT
========================================================================= */

#include "unity.h"
#include "base_components/encoder.h"
#include "Mockgpio.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void _encoder_gpio_callback(hal_gpio_pin_t pin, void *arg);
void test_encoder_init_sets_states(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  encoder.pin_a = 1;
  encoder.pin_b = 2;
  encoder.pin_sw = 3;

  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_sw, 1);

  hal_gpio_callback_Expect(encoder.pin_a, _encoder_gpio_callback, &encoder);
  hal_gpio_callback_Expect(encoder.pin_b, _encoder_gpio_callback, &encoder);
  hal_gpio_callback_Expect(encoder.pin_sw, _encoder_gpio_callback, &encoder);

  // Run Test
  encoder_init(&encoder);

  // All pins current state should be set to 1/high
  TEST_ASSERT_EQUAL(1, encoder.pin_a_state);
  TEST_ASSERT_EQUAL(0, encoder.pin_b_state);
  TEST_ASSERT_EQUAL(1, encoder.pin_sw_state);
}

gpio_callback_t pin_a_callback;
hal_gpio_pin_t gpio_pin_a;
void *pin_a_arg;
void trigger_pin_change()
{
  pin_a_callback(gpio_pin_a, pin_a_arg);
}

void captured_hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback, void *arg, int cmock_num_calls)
{
  gpio_pin_a = gpio_pin;
  pin_a_callback = callback;
  pin_a_arg = arg;
}

void test_encoder_on_pin_change(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  encoder.pin_a = 1;
  encoder.pin_b = 2;
  encoder.pin_sw = 3;

  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_sw, 1);

  hal_gpio_callback_Expect(encoder.pin_a, _encoder_gpio_callback, &encoder);
  hal_gpio_callback_Expect(encoder.pin_b, _encoder_gpio_callback, &encoder);
  hal_gpio_callback_Expect(encoder.pin_sw, _encoder_gpio_callback, &encoder);

  hal_gpio_callback_AddCallback(captured_hal_gpio_callback);

  hal_gpio_read_ExpectAndReturn(encoder.pin_sw, 0);

  // Run Test
  encoder_init(&encoder);
  encoder.pin_sw_last_change = -100; // TEMP: force last change to be over 100ms ago

  trigger_pin_change();
}
