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

  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);  // Return 1 for pin_a
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);  // Return 1 for pin_b
  hal_gpio_read_ExpectAndReturn(encoder.pin_sw, 1); // Return 1 for pin_sw

  hal_gpio_callback_Expect(encoder.pin_a, _encoder_gpio_callback, &encoder);
  hal_gpio_callback_Expect(encoder.pin_b, _encoder_gpio_callback, &encoder);
  hal_gpio_callback_Expect(encoder.pin_sw, _encoder_gpio_callback, &encoder);

  // Run Test
  encoder_init(&encoder);

  // All pins current state should be set to 1/high
  TEST_ASSERT_EQUAL(1, encoder.pin_a_state);
  TEST_ASSERT_EQUAL(1, encoder.pin_b_state);
  TEST_ASSERT_EQUAL(0, encoder.pin_sw_state);
}
