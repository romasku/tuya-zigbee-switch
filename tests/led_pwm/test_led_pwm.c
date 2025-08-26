#include "unity/unity.h"
#include "base_components/led_pwm.h"
#include "device_config/pwm_nv.h"
#include "stubs/gpio.h"
#include "stubs/clock.h"
#include <string.h>

#define INDICATOR_PWM_SUPPORT

#define ZCL_ONOFF_INDICATOR_MODE_NORMAL   0
#define ZCL_ONOFF_INDICATOR_MODE_OPPOSITE 1
#define ZCL_ONOFF_INDICATOR_MODE_MANUAL   2

led_t leds[5] = 
{
  {.pin = 0xD3, .on_high = 1, .on = 0},
  {.pin = 0xC0, .on_high = 1, .on = 0},
  {.pin = 0xB5, .on_high = 1, .on = 0},
};
u8 leds_cnt = 3;
typedef struct
{
  u8 endpoint;
  led_t *indicator_led;
  u8 indicator_led_mode;
#ifdef INDICATOR_PWM_SUPPORT
  u8 pwm_enabled;
  u8 pwm_brightness;
  u8 pwm_saved_state;
#endif
} zigbee_relay_cluster;

zigbee_relay_cluster relay_clusters[4] = 
{
  {.endpoint = 0, .indicator_led = &leds[0], .indicator_led_mode = ZCL_ONOFF_INDICATOR_MODE_NORMAL, .pwm_enabled = 0, .pwm_brightness = 2},
  {.endpoint = 1, .indicator_led = &leds[1], .indicator_led_mode = ZCL_ONOFF_INDICATOR_MODE_NORMAL, .pwm_enabled = 0, .pwm_brightness = 3},
  {.endpoint = 2, .indicator_led = NULL, .indicator_led_mode = ZCL_ONOFF_INDICATOR_MODE_NORMAL, .pwm_enabled = 0, .pwm_brightness = 0},
  {.endpoint = 3, .indicator_led = NULL, .indicator_led_mode = ZCL_ONOFF_INDICATOR_MODE_NORMAL, .pwm_enabled = 0, .pwm_brightness = 0}
};
u8 relay_clusters_cnt = 2;
#ifdef INDICATOR_PWM_SUPPORT
u8 relay_cluster_get_pwm_brightness(zigbee_relay_cluster *cluster)
{
  return cluster->pwm_brightness;
}

void relay_cluster_set_pwm_brightness(zigbee_relay_cluster *cluster, u8 brightness)
{
  if (brightness > 15)
  {
    brightness = 15;
  }
  
  cluster->pwm_brightness = brightness;
  
  pwm_nv_config_t config;
  config.pwm_enabled = cluster->pwm_enabled;
  config.pwm_brightness = cluster->pwm_brightness;
  pwm_nv_write_config(cluster->endpoint, &config);
}

void relay_cluster_enable_pwm(zigbee_relay_cluster *cluster, u8 enable)
{
  cluster->pwm_enabled = enable ? 1 : 0;
  
  pwm_nv_config_t config;
  config.pwm_enabled = cluster->pwm_enabled;
  config.pwm_brightness = cluster->pwm_brightness;
  pwm_nv_write_config(cluster->endpoint, &config);
}

u8 relay_cluster_is_pwm_enabled(zigbee_relay_cluster *cluster)
{
  return cluster->pwm_enabled;
}

void relay_cluster_save_pwm_state_for_blink(zigbee_relay_cluster *cluster)
{
  cluster->pwm_saved_state = cluster->pwm_brightness;
}

void relay_cluster_restore_pwm_state_after_blink(zigbee_relay_cluster *cluster)
{
  cluster->pwm_brightness = cluster->pwm_saved_state;
}

void relay_cluster_led_blink(zigbee_relay_cluster *cluster, u16 on_time_ms, u16 off_time_ms, u16 times)
{
  if (cluster->indicator_led)
  {
    relay_cluster_save_pwm_state_for_blink(cluster);
    cluster->indicator_led->blink_times_left = times;
  }
}

void sync_indicator_led(zigbee_relay_cluster *cluster)
{
  if (cluster->indicator_led && cluster->indicator_led->blink_times_left == 0)
  {
    if (cluster->pwm_enabled && cluster->pwm_brightness > 0)
    {
    }
    else
    {
    }
  }
}

// Mock NV storage functions
typedef enum {
  NV_SUCC = 0,
  NV_FAIL = 1
} nv_sts_t;

#define NV_MODULE_ZCL 1

nv_sts_t nv_flashReadNew(u8 op, u8 module, u16 id, u16 len, u8 *buf)
{
  return NV_FAIL;
}

nv_sts_t nv_flashWriteNew(u8 op, u8 module, u16 id, u16 len, u8 *buf)
{
  return NV_SUCC;
}

void nv_flashSingleItemRemove(u8 module, u16 id, u16 len)
{
}

#endif

void test_pwm_registration(void)
{
  pwm_led_count = 0;
  memset(pwm_led_registry, 0, sizeof(pwm_led_registry));
  
  TEST_ASSERT_EQUAL(1, led_pwm_register_led(0, 2));
  TEST_ASSERT_EQUAL(1, led_pwm_register_led(1, 3));
  TEST_ASSERT_EQUAL(2, pwm_led_count);
  
  TEST_ASSERT_EQUAL(1, led_pwm_is_registered(0));
  TEST_ASSERT_EQUAL(1, led_pwm_is_registered(1));
  TEST_ASSERT_EQUAL(0, led_pwm_is_registered(2));
  
  TEST_ASSERT_EQUAL(2, led_pwm_get_default_brightness(0));
  TEST_ASSERT_EQUAL(3, led_pwm_get_default_brightness(1));
  TEST_ASSERT_EQUAL(0, led_pwm_get_default_brightness(2));
}

void test_pwm_brightness_control(void)
{
  led_pwm_init();
  
  led_pwm_enable(0, 4);
  led_pwm_enable(1, 8);
  
  led_pwm_set_brightness(0, 0);
  led_pwm_set_brightness(1, 15);
}

void test_pwm_timing(void)
{
  led_pwm_init();
  led_pwm_enable(0, 4);
  
  for (int cycle = 0; cycle < 2; cycle++)
  {
    for (int step = 0; step < 16; step++)
    {
      led_pwm_timer_handler();
      
      u8 expected_state = (step < 4) ? 1 : 0;
    }
  }
}

void test_pwm_state_save_restore(void)
{
  led_pwm_init();
  led_pwm_enable(0, 6);
  
  led_pwm_save_state(0);
  
  led_pwm_set_brightness(0, 10);
  
  led_pwm_restore_state(0);
}

void test_pwm_enhanced_features(void)
{
  TEST_ASSERT_EQUAL(1, led_pwm_validate_state());
  
  led_pwm_channel_t channel_info;
  led_pwm_init();
  led_pwm_enable(0, 8);
  
  TEST_ASSERT_EQUAL(1, led_pwm_get_channel_info(0, &channel_info));
  TEST_ASSERT_EQUAL(0, channel_info.led_index);
  TEST_ASSERT_EQUAL(1, channel_info.enabled);
  
  TEST_ASSERT_EQUAL(0, led_pwm_get_channel_info(2, &channel_info));
  
  led_pwm_reset_manager();
  TEST_ASSERT_EQUAL(0, led_pwm_get_channel_info(0, &channel_info));
}

void test_pwm_gamma_correction(void)
{
  led_pwm_init();
  led_pwm_enable(0, 4);
  
  led_pwm_channel_t channel_info;
  if (led_pwm_get_channel_info(0, &channel_info))
  {
  }
}

void test_pwm_relay_integration(void)
{
  relay_cluster_enable_pwm(&relay_clusters[0], 1);
  TEST_ASSERT_EQUAL(1, relay_cluster_is_pwm_enabled(&relay_clusters[0]));
  
  relay_cluster_set_pwm_brightness(&relay_clusters[0], 8);
  TEST_ASSERT_EQUAL(8, relay_cluster_get_pwm_brightness(&relay_clusters[0]));
  
  relay_cluster_set_pwm_brightness(&relay_clusters[0], 20);
  TEST_ASSERT_EQUAL(15, relay_cluster_get_pwm_brightness(&relay_clusters[0]));
}

void test_pwm_blink_integration(void)
{
  relay_cluster_enable_pwm(&relay_clusters[0], 1);
  relay_cluster_set_pwm_brightness(&relay_clusters[0], 10);
  
  relay_cluster_led_blink(&relay_clusters[0], 100, 100, 3);
  
  TEST_ASSERT_GREATER_THAN(0, relay_clusters[0].indicator_led->blink_times_left);
  
  TEST_ASSERT_EQUAL(10, relay_clusters[0].pwm_saved_state);
  
  relay_clusters[0].indicator_led->blink_times_left = 0;
  
  sync_indicator_led(&relay_clusters[0]);
}

void test_pwm_indicator_modes(void)
{
  relay_clusters[0].indicator_led_mode = ZCL_ONOFF_INDICATOR_MODE_NORMAL;
  relay_cluster_enable_pwm(&relay_clusters[0], 1);
  relay_cluster_set_pwm_brightness(&relay_clusters[0], 5);
  
  relay_clusters[0].indicator_led_mode = ZCL_ONOFF_INDICATOR_MODE_OPPOSITE;
  sync_indicator_led(&relay_clusters[0]);
  
  relay_clusters[0].indicator_led_mode = ZCL_ONOFF_INDICATOR_MODE_MANUAL;
  sync_indicator_led(&relay_clusters[0]);
}

void test_timer_resource_management(void)
{
  u8 available = led_pwm_check_timer_availability();
  TEST_ASSERT_EQUAL(0, available);
  
  TEST_ASSERT_EQUAL(1, led_pwm_reserve_timer());
  led_pwm_release_timer();
  
  led_pwm_register_led(0, 4);
  led_pwm_init();
  led_pwm_enable(0, 8);
  led_pwm_disable(0);
  led_pwm_deinit();
}

void test_pwm_nv_storage(void)
{
  pwm_nv_config_t config;
  u8 result = pwm_nv_read_config(1, &config);
  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_EQUAL(0, config.pwm_enabled);
  TEST_ASSERT_EQUAL(2, config.pwm_brightness);
  
  config.pwm_enabled = 1;
  config.pwm_brightness = 8;
  pwm_nv_write_config(1, &config);
  
  config.pwm_brightness = 20;
  pwm_nv_write_config(1, &config);
  
  relay_cluster_set_pwm_brightness(&relay_clusters[0], 8);
  relay_cluster_enable_pwm(&relay_clusters[0], 1);
  
  TEST_ASSERT_EQUAL(8, relay_clusters[0].pwm_brightness);
  TEST_ASSERT_EQUAL(1, relay_clusters[0].pwm_enabled);
}

int main(void)
{
  UNITY_BEGIN();
  
  RUN_TEST(test_pwm_registration);
  RUN_TEST(test_pwm_brightness_control);
  RUN_TEST(test_pwm_timing);
  RUN_TEST(test_pwm_state_save_restore);
  RUN_TEST(test_pwm_enhanced_features);
  RUN_TEST(test_pwm_gamma_correction);
  RUN_TEST(test_pwm_relay_integration);
  RUN_TEST(test_pwm_blink_integration);
  RUN_TEST(test_pwm_indicator_modes);
  RUN_TEST(test_timer_resource_management);
  RUN_TEST(test_pwm_nv_storage);
  
  return UNITY_END();
}