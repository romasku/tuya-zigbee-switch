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

int on_rotate_cw_calls = 0;
void on_rotate_cw(void)
{
  on_rotate_cw_calls++;
}

void setUp(void)
{
  // Put a space between tests for readability
  printf("\r\n");

  // Reset
  on_rotate_ccw_calls = 0;
  on_rotate_cw_calls = 0;
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

void _setup_encoder(encoder_t *encoder, uint8_t initial_a_state, uint8_t initial_b_state, uint8_t initial_sw_state)
{
  // Setup Encoder
  encoder->pin_a = 1;
  encoder->pin_b = 2;
  encoder->pin_sw = 3;
  encoder->on_rotate_ccw = on_rotate_ccw;
  encoder->on_rotate_cw = on_rotate_cw;

  hal_gpio_read_ExpectAndReturn(encoder->pin_a, initial_a_state);
  hal_gpio_read_ExpectAndReturn(encoder->pin_b, initial_b_state);
  hal_gpio_read_ExpectAndReturn(encoder->pin_sw, initial_sw_state);

  hal_millis_IgnoreAndReturn(10);

  hal_gpio_callback_StubWithCallback(captured_hal_gpio_callback);

  encoder_init(encoder);
}

void _trigger_pin_change(int callback_cnt, hal_gpio_pin_t pin, uint8_t new_state, uint32_t time_of_change)
{
  // Prep For pin a changing
  hal_millis_IgnoreAndReturn(time_of_change);
  hal_gpio_read_ExpectAndReturn(pin, new_state);

  // Trigger gpio change call back
  trigger_pin_change(callback_cnt);
}

// When Pin A changes from high to low, before pin b, we should see this as Rotating CCW
void test_encoder_pin_a_changing_before_pin_b(void)
{
  // Setup Encoder, with all pins high
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1, 1, 1);

  // Trigger pin a change, to low, 100ms later
  _trigger_pin_change(0, encoder.pin_a, 0, 110);

  // on_rotate_ccw was called once
  TEST_ASSERT_EQUAL(1, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
}

// When Pin A changes from high to low, after pin b, we should do nothing
void test_encoder_pin_a_changing_after_pin_b(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1, 0, 1); // Pin B starts low

  // Trigger pin a change, to low, 100ms later
  _trigger_pin_change(0, encoder.pin_a, 0, 110);

  // no callbacks called
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
}

// When pin B changes from high to low, before pin A, Rotating CW callback should be triggered
void test_encoder_pin_b_changing_before_pin_a(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1, 1, 1);

  // Trigger pin b change, to low, 100ms later
  _trigger_pin_change(1, encoder.pin_b, 0, 110);

  // no callbacks called
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(1, on_rotate_cw_calls);
}

// When pin B changes to low, after pin A, we should do nothing
void test_encoder_pin_b_changing_after_pin_a(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 0, 1, 1);

  // Trigger pin b change, to low, 100ms later
  _trigger_pin_change(1, encoder.pin_b, 0, 110);

  // no callbacks called
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
}
