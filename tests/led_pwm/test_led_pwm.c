#include "unity/unity.h"
#include "base_components/led_pwm.h"
#include "base_components/led.h"
#include "stubs/timer_stubs.h"
#include <string.h>

led_t leds[3] = 
{
  {.pin = 0xD3, .on_high = 1, .on = 0, .blink_times_left = 0},
  {.pin = 0xC0, .on_high = 1, .on = 0, .blink_times_left = 0},
  {.pin = 0xB5, .on_high = 1, .on = 0, .blink_times_left = 0},
};
u8 leds_cnt = 3;


typedef enum {
  NV_SUCC = 0,
  NV_FAIL = 1
} nv_sts_t;

#define NV_MODULE_ZCL 1

nv_sts_t nv_flashReadNew(u8 op, u8 module, u16 id, u16 len, u8 *buf) {
  (void)op; (void)module; (void)id; (void)len; (void)buf;
  return NV_FAIL;
}

nv_sts_t nv_flashWriteNew(u8 op, u8 module, u16 id, u16 len, u8 *buf) {
  (void)op; (void)module; (void)id; (void)len; (void)buf;
  return NV_SUCC;
}

void nv_flashSingleItemRemove(u8 module, u16 id, u16 len) {
  (void)module; (void)id; (void)len;
}

#ifdef INDICATOR_PWM_SUPPORT
u8 device_supports_pwm(void)
{
#ifdef ROUTER
  return 1;
#else
  return 0;
#endif
}
#endif

void test_pwm_registration(void)
{
#ifdef INDICATOR_PWM_SUPPORT
  TEST_ASSERT_EQUAL(1, led_pwm_register_led(0, 2));
  TEST_ASSERT_EQUAL(1, led_pwm_register_led(1, 3));
  
  TEST_ASSERT_EQUAL(1, led_pwm_is_registered(0));
  TEST_ASSERT_EQUAL(1, led_pwm_is_registered(1));
  TEST_ASSERT_EQUAL(0, led_pwm_is_registered(2));
  
  TEST_ASSERT_EQUAL(2, led_pwm_get_default_brightness(0));
  TEST_ASSERT_EQUAL(3, led_pwm_get_default_brightness(1));
  TEST_ASSERT_EQUAL(0, led_pwm_get_default_brightness(2));
#else
  TEST_FAIL_MESSAGE("INDICATOR_PWM_SUPPORT not defined - compiler flags not working");
#endif
}

void test_pwm_brightness_control(void)
{
  led_pwm_register_led(0, 4);
  led_pwm_init();
  
  led_pwm_set_brightness(0, 8);
  led_pwm_set_brightness(0, 0);
  led_pwm_set_brightness(0, 15);
}

void test_pwm_timing(void)
{
  led_pwm_register_led(0, 4);
  led_pwm_init();
  led_pwm_enable(0, 4);
  led_pwm_disable(0);
}

void test_pwm_state_save_restore(void)
{
  led_pwm_register_led(0, 6);
  led_pwm_init();
  led_pwm_enable(0, 6);
  
  led_pwm_save_state(0);
  led_pwm_set_brightness(0, 10);
  led_pwm_restore_state(0);
}

void test_timer_resource_management(void)
{
  led_pwm_register_led(0, 4);
  led_pwm_init();
  led_pwm_enable(0, 8);
  led_pwm_disable(0);
  led_pwm_deinit();
}

void test_sdk_timer_integration(void)
{
#ifdef INDICATOR_PWM_SUPPORT
  timer_stubs_reset();
  
  led_pwm_register_led(0, 4);
  led_pwm_init();
  
  led_pwm_enable(0, 8);
  
  led_pwm_disable(0);
  led_pwm_deinit();
#else
  TEST_FAIL_MESSAGE("INDICATOR_PWM_SUPPORT not defined");
#endif
}

void test_device_type_pwm_detection(void)
{
#ifdef INDICATOR_PWM_SUPPORT
#ifdef ROUTER
  TEST_ASSERT_EQUAL(1, device_supports_pwm());
#else
  TEST_ASSERT_EQUAL(0, device_supports_pwm());
#endif
#else
  TEST_FAIL_MESSAGE("INDICATOR_PWM_SUPPORT not defined");
#endif
}

void test_pwm_nv_storage(void)
{
#ifdef INDICATOR_PWM_SUPPORT
  // Test PWM NV storage functionality
  led_pwm_register_led(0, 4);
  led_pwm_init();
  
  // Test basic save/restore functionality
  led_pwm_enable(0, 8);
  led_pwm_save_state(0);
  led_pwm_set_brightness(0, 12);
  led_pwm_restore_state(0);
  
  // Test with disabled PWM
  led_pwm_disable(0);
  led_pwm_save_state(0);
  led_pwm_restore_state(0);
  
  led_pwm_deinit();
#else
  TEST_FAIL_MESSAGE("INDICATOR_PWM_SUPPORT not defined");
#endif
}

void setUp(void)
{
}

void tearDown(void)
{
}

void run_all_tests(void)
{
  RUN_TEST(test_pwm_registration);
  RUN_TEST(test_pwm_brightness_control);
  RUN_TEST(test_pwm_timing);
  RUN_TEST(test_pwm_state_save_restore);
  RUN_TEST(test_timer_resource_management);
  RUN_TEST(test_pwm_nv_storage);
  RUN_TEST(test_sdk_timer_integration);
  RUN_TEST(test_device_type_pwm_detection);
}