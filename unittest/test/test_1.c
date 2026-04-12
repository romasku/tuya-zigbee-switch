/* =========================================================================
    Unity - A Test Framework for C
    ThrowTheSwitch.org
    Copyright (c) 2007-26 Mike Karlesky, Mark VanderVoord, & Greg Williams
    SPDX-License-Identifier: MIT
========================================================================= */

#include "unity.h"
#include "base_components/encoder.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_encoder_init_sets_states(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  encoder.pin_a = 1;
  encoder.pin_b = 2;
  encoder.pin_sw = 3;

  // Run Test
  encoder_init(&encoder);

  // All pins current state should be set to 1/high
  TEST_ASSERT_EQUAL(1, encoder.pin_a_state);
  TEST_ASSERT_EQUAL(1, encoder.pin_b_state);
  TEST_ASSERT_EQUAL(1, encoder.pin_sw_state);
}
