#include "unity.h"
#include "Mockgpio.h"
#include "Mocktimer.h"
#include "base_components/encoder.h"
#include "gpio_callback_helper.h"

int on_rotate_ccw_calls = 0;
void on_rotate_ccw(void)
{
  on_rotate_ccw_calls++;
}

void setUp(void)
{
  // Put a space between tests for readability
  printf("\r\n");

  on_rotate_ccw_calls = 0; // Reset
}

void tearDown(void)
{
}

void test_encoder_init_sets_states(void)
{
  // Check that the initial state of the input pins is read and saved

  // Setup Encoder
  encoder_t encoder = {};
  encoder.pin_a = 1;
  encoder.pin_b = 2;
  encoder.pin_sw = 3;

  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_sw, 1);

  hal_gpio_callback_Ignore();

  hal_millis_IgnoreAndReturn(10);

  // Run Test
  encoder_init(&encoder);

  // All pins current state should be set to 1/high
  TEST_ASSERT_EQUAL(1, encoder.pin_a_state);
  TEST_ASSERT_EQUAL(0, encoder.pin_b_state);
  TEST_ASSERT_EQUAL(1, encoder.pin_sw_state);
}

// When Pin A changes from high to low, before pin b, we should see this as Rotating CCW
void test_encoder_pin_a_changing_before_pin_b(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  encoder.pin_a = 1;
  encoder.pin_b = 2;
  encoder.pin_sw = 3;
  encoder.on_rotate_ccw = on_rotate_ccw;

  hal_gpio_read_IgnoreAndReturn(1);
  hal_millis_IgnoreAndReturn(10);
  hal_gpio_callback_StubWithCallback(captured_hal_gpio_callback);

  encoder_init(&encoder);

  // Prep For pin a changing to 0/low
  hal_millis_IgnoreAndReturn(110); // Move time on 100ms
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);

  // Trigger pin A change call back
  trigger_pin_change(0);

  // on_rotate_ccw was called once
  TEST_ASSERT_EQUAL(1, on_rotate_ccw_calls);
}

// When Pin A changes from high to low, after pin b, we should do nothing
void test_encoder_pin_a_changing_after_pin_b(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  encoder.pin_a = 1;
  encoder.pin_b = 2;
  encoder.pin_sw = 3;
  encoder.on_rotate_ccw = on_rotate_ccw;

  hal_millis_IgnoreAndReturn(10);
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 1); // Pin B is already high
  hal_gpio_read_ExpectAndReturn(encoder.pin_sw, 0);
  hal_gpio_callback_StubWithCallback(captured_hal_gpio_callback);

  encoder_init(&encoder);

  // Prep For pin a changing to 0/low
  hal_millis_IgnoreAndReturn(110); // Move time on 100ms
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);

  // Trigger pin A change call back
  trigger_pin_change(0);

  // on_rotate_ccw was not called
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
}
