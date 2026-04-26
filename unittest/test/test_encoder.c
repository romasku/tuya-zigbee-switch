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

int on_press_calls = 0;
void on_press(void)
{
  on_press_calls++;
}

int on_rotate_ccw_while_pressed_calls = 0;
void on_rotate_ccw_while_pressed(void)
{
  on_rotate_ccw_while_pressed_calls++;
}

int on_rotate_cw_while_pressed_calls = 0;
void on_rotate_cw_while_pressed(void)
{
  on_rotate_cw_while_pressed_calls++;
}

void setUp(void)
{
  // Put a space between tests for readability
  printf("\r\n");

  // Reset
  on_rotate_ccw_calls = 0;
  on_rotate_cw_calls = 0;
  on_press_calls = 0;
  on_rotate_ccw_while_pressed_calls = 0;
  on_rotate_cw_while_pressed_calls = 0;
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

  hal_gpio_read_ExpectAndReturn(encoder.pin_sw, 1);

  hal_gpio_callback_Ignore(); // Other test ensure callbacks are set correctly

  hal_millis_IgnoreAndReturn(10);

  // Run Test
  encoder_init(&encoder);

  // SW pin set correctly
  TEST_ASSERT_EQUAL(1, encoder.pin_sw_state);
  TEST_ASSERT_EQUAL(10, encoder.pin_sw_last_change);

  // Values for encoder rotation state tracking are set correctly
  TEST_ASSERT_EQUAL(0b00000011, encoder.old_AB);
  TEST_ASSERT_EQUAL(0, encoder.encval);

  TEST_ASSERT_EQUAL(false, encoder.rotate_since_pressed);
}

void _setup_encoder(encoder_t *encoder, uint8_t initial_sw_state)
{
  // Setup Encoder
  encoder->pin_a = 1;
  encoder->pin_b = 2;
  encoder->pin_sw = 3;
  encoder->on_rotate_ccw = (ev_encoder_callback_t)on_rotate_ccw;
  encoder->on_rotate_cw = (ev_encoder_callback_t)on_rotate_cw;
  encoder->on_press = (ev_encoder_callback_t)on_press;
  encoder->on_rotate_ccw_while_pressed = (ev_encoder_callback_t)on_rotate_ccw_while_pressed;
  encoder->on_rotate_cw_while_pressed = (ev_encoder_callback_t)on_rotate_cw_while_pressed;

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

// When Pin A changes from high to low, before pin b, we should see this as Rotating CW
void test_encoder_pin_a_changing_before_pin_b(void)
{
  // Setup Encoder, with all pins high
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1);

  // Pin A changes first
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 1);
  trigger_pin_change(1);
  
  // Then Pin B catches up
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  trigger_pin_change(1);

  // on_rotate_cw was called once
  TEST_ASSERT_EQUAL(1, on_rotate_cw_calls);

  // no other callbacks triggered 
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_press_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_while_pressed_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}

// When Pin A changes from high to low, and then back again, we should do nothing
void test_encoder_only_pin_a_changes(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1);

  // Pin A changes
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 1);
  trigger_pin_change(1);

  // Pin A changes back
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 1);
  trigger_pin_change(1);

  // no callbacks called
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_press_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_while_pressed_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}

// When pin B changes from high to low, before pin A, Rotating CW callback should be triggered
void test_encoder_pin_b_changing_before_pin_a(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1);

  // Pin B changes first
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  trigger_pin_change(1);
  
  // Then Pin A catches up
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  trigger_pin_change(1);

  // on_rotate_ccw was called once
  TEST_ASSERT_EQUAL(1, on_rotate_ccw_calls);

  // no other callbacks triggered 
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_press_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_while_pressed_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}

// When pin B changes to low, and then back again, we should do nothing
void test_encoder_only_pin_b_changes(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1);

  // Pin B changes
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  trigger_pin_change(1);

  // Pin B changes back
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 1);
  trigger_pin_change(1);

  // no callbacks called
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_press_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_while_pressed_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}

void test_encoder_pin_sw_changes_to_low(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1);

  // Trigger pin sw change, to low, 100ms later
  _trigger_pin_change(2, encoder.pin_sw, 0, 110);

  // Not callbacks triggered (we trigger on press cb on release)
  TEST_ASSERT_EQUAL(0, on_press_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_while_pressed_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}

void test_encoder_pin_sw_changes_to_high(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 0);

  // Trigger pin sw change, to high, 100ms later
  _trigger_pin_change(2, encoder.pin_sw, 1, 110);

  // On Press Callback triggered
  TEST_ASSERT_EQUAL(1, on_press_calls);

  // no other callbacks called
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_while_pressed_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}

// Pin changes are often noisy, this checks the debouncing logic filters the extra events
void test_encoder_sw_pressed_noisy(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1);

  // Trigger pin sw change, to low, 100ms later
  // Then retrigger a few times, as if the input is noisy
  _trigger_pin_change(2, encoder.pin_sw, 0, 110);
  _trigger_pin_change(2, encoder.pin_sw, 1, 111);
  _trigger_pin_change(2, encoder.pin_sw, 0, 112);
  _trigger_pin_change(2, encoder.pin_sw, 1, 113);
  _trigger_pin_change(2, encoder.pin_sw, 0, 116);
  _trigger_pin_change(2, encoder.pin_sw, 0, 119);

  // And then back to high, again with noise
  _trigger_pin_change(2, encoder.pin_sw, 1, 300);
  _trigger_pin_change(2, encoder.pin_sw, 0, 301);
  _trigger_pin_change(2, encoder.pin_sw, 1, 303);
  _trigger_pin_change(2, encoder.pin_sw, 0, 305);
  _trigger_pin_change(2, encoder.pin_sw, 1, 307);
  _trigger_pin_change(2, encoder.pin_sw, 1, 309);

  // On Press Callback triggered - Only once!
  TEST_ASSERT_EQUAL(1, on_press_calls);

  // no other callbacks called
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_while_pressed_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}

// When Pin A changes from high to low, before pin b, while sw is low, we should see this as Rotating CW while pressed
void test_encoder_pin_a_changing_before_pin_b_while_sw_is_low(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 0);

  // Pin A changes first
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 1);
  trigger_pin_change(1);
  
  // Then Pin B catches up
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  trigger_pin_change(1);

  // CCW while pressed triggered
  TEST_ASSERT_EQUAL(1, on_rotate_cw_while_pressed_calls);

  // No other callbacks triggered
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_press_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}

// When pin B changes from high to low, before pin A, Rotating CCW while pressed callback should be triggered
void test_encoder_pin_b_changing_before_pin_a_while_sw_is_low(void)
{
  // Setup Encoder
  encoder_t encoder = {};
  _setup_encoder(&encoder, 0);

  // Pin B changes first
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  trigger_pin_change(1);
  
  // Then Pin A catches up
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  trigger_pin_change(1);

  // CW while pressed triggered
  TEST_ASSERT_EQUAL(1, on_rotate_ccw_while_pressed_calls);

  // No other callbacks triggered
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_press_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_while_pressed_calls);
}

// When sw is pressed and then rotated CW, pressed should not be triggered
void test_encoder_pressed_and_rotated__pressed_cb_not_triggered(void)
{
  // Setup Encoder, with all pins high
  encoder_t encoder = {};
  _setup_encoder(&encoder, 1);

  // Press Encoder
  _trigger_pin_change(2, encoder.pin_sw, 0, 100);

  // Rotate CW
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 1);
  trigger_pin_change(1);
  hal_gpio_read_ExpectAndReturn(encoder.pin_a, 0);
  hal_gpio_read_ExpectAndReturn(encoder.pin_b, 0);
  trigger_pin_change(1);

  // Release Encoder
  _trigger_pin_change(2, encoder.pin_sw, 1, 400);

  // Action was seen as a roate cw while pressed, not a on press ... or any other event
  TEST_ASSERT_EQUAL(1, on_rotate_cw_while_pressed_calls);
  TEST_ASSERT_EQUAL(0, on_press_calls);

  TEST_ASSERT_EQUAL(0, on_rotate_ccw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_cw_calls);
  TEST_ASSERT_EQUAL(0, on_rotate_ccw_while_pressed_calls);
}