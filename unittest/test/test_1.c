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

  hal_gpio_callback_Ignore();

  // Run Test
  encoder_init(&encoder);

  // All pins current state should be set to 1/high
  TEST_ASSERT_EQUAL(1, encoder.pin_a_state);
  TEST_ASSERT_EQUAL(0, encoder.pin_b_state);
  TEST_ASSERT_EQUAL(1, encoder.pin_sw_state);
}

gpio_callback_t gpio_callbacks[3];
hal_gpio_pin_t gpio_pin_a[3];
void *pin_a_arg[3];
void trigger_pin_change()
{
  gpio_callbacks[0](gpio_pin_a[0], pin_a_arg[0]);
}

void captured_hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback, void *arg, int cmock_num_calls)
{
  gpio_pin_a[cmock_num_calls] = gpio_pin;
  gpio_callbacks[cmock_num_calls] = callback;
  pin_a_arg[cmock_num_calls] = arg;
}

void test_encoder_on_pin_change(void)
{
  // Test Pin 1 starting high and then changing to low

  // Setup Encoder
  encoder_t encoder = {};
  encoder.pin_a = 1;
  encoder.pin_b = 2;
  encoder.pin_sw = 3;

  hal_gpio_read_IgnoreAndReturn(1);

  hal_gpio_callback_StubWithCallback(captured_hal_gpio_callback);

  encoder_init(&encoder);

  TEST_ASSERT_EQUAL(1, encoder.pin_a_state);

  // Prep For pin a changing to 0/low
  encoder.pin_a_last_change = -100; // TEMP: force last change to be over 100ms ago
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);

  // Trigger pin change call back
  trigger_pin_change();

  // Pin A state is now low
  TEST_ASSERT_EQUAL(0, encoder.pin_a_state);
}
