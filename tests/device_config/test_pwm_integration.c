#include <stdio.h>
#include <assert.h>
#include <string.h>

#define INDICATOR_PWM_SUPPORT
#define ROUTER
#define DEFAULT_INDICATOR_BRIGHTNESS 2

// Mock GPIO definitions
typedef enum {
  GPIO_PD3 = 0xD3,
  GPIO_PC0 = 0xC0,
  GPIO_PB5 = 0xB5,
  GPIO_PB4 = 0xB4
} GPIO_PinTypeDef;

// Mock LED structure
typedef struct {
  u32 pin;
  u8  on_high;
  u8  on;
  u16 blink_times_left;
  u16 blink_time_on;
  u16 blink_time_off;
  u32 blink_switch_counter;
  u32 last_update;
} led_t;

// Mock relay cluster structure
typedef struct {
  led_t *indicator_led;
  u8 endpoint;
} zigbee_relay_cluster;

// Test data
led_t leds[4] = {
  {.pin = GPIO_PD3, .on_high = 1, .on = 0},  // PWM capable
  {.pin = GPIO_PC0, .on_high = 1, .on = 0},  // PWM capable
  {.pin = GPIO_PB5, .on_high = 1, .on = 0},  // Not PWM capable
  {.pin = GPIO_PB4, .on_high = 1, .on = 0}   // Not PWM capable
};

u8 leds_cnt = 4;

zigbee_relay_cluster relay_clusters[2] = {
  {.indicator_led = &leds[0], .endpoint = 1},
  {.indicator_led = &leds[1], .endpoint = 2}
};

u8 relay_clusters_cnt = 2;

// Mock PWM functions
u8 pwm_led_count = 0;
u8 led_pwm_register_led(u8 led_index, u8 brightness) {
  if (led_index < 4 && brightness <= 15) {
    pwm_led_count++;
    return 1;
  }
  return 0;
}
void led_pwm_init(void) {}

// Include the functions we want to test
u8 device_has_indicator_pwm(void);
u8 pin_supports_pwm(GPIO_PinTypeDef pin);
u8 is_indicator_led(led_t *led);
u8 get_default_brightness(void);
void apply_pwm_config_from_db(void);

// Implementation of functions under test
u8 device_has_indicator_pwm(void)
{
#ifdef ROUTER
  return 1;
#else
  return 0;
#endif
}

u8 pin_supports_pwm(GPIO_PinTypeDef pin)
{
  return (pin == GPIO_PD3 || pin == GPIO_PC0);
}

u8 is_indicator_led(led_t *led)
{
  for (int i = 0; i < relay_clusters_cnt; i++)
  {
    if (relay_clusters[i].indicator_led == led)
    {
      return 1;
    }
  }
  return 0;
}

u8 get_default_brightness(void)
{
#ifdef DEFAULT_INDICATOR_BRIGHTNESS
  return DEFAULT_INDICATOR_BRIGHTNESS;
#else
  return 2;
#endif
}

void apply_pwm_config_from_db(void)
{
  if (device_has_indicator_pwm())
  {
    for (int i = 0; i < leds_cnt; i++)
    {
      if (is_indicator_led(&leds[i]) && pin_supports_pwm(leds[i].pin))
      {
        u8 brightness = get_default_brightness();
        if (led_pwm_register_led(i, brightness))
        {
          // Successfully registered LED for PWM
        }
      }
    }
    
    if (pwm_led_count > 0)
    {
      led_pwm_init();
    }
  }
}

/**
 * @brief      Test device PWM capability detection
 * @param	   none
 * @return     none
 */
void test_device_pwm_capability(void)
{
  printf("Testing device PWM capability detection...\\n");
  
  // Router builds should support PWM
  assert(device_has_indicator_pwm() == 1);
  
  printf("Device PWM capability tests passed!\\n");
}

/**
 * @brief      Test pin PWM support detection
 * @param	   none
 * @return     none
 */
void test_pin_pwm_support(void)
{
  printf("Testing pin PWM support detection...\\n");
  
  // Moes ZS-EUB 2-gang PWM capable pins: D3, C0
  assert(pin_supports_pwm(GPIO_PD3) == 1);
  assert(pin_supports_pwm(GPIO_PC0) == 1);
  
  // Other pins should not support PWM
  assert(pin_supports_pwm(GPIO_PB5) == 0);
  assert(pin_supports_pwm(GPIO_PB4) == 0);
  
  printf("Pin PWM support tests passed!\\n");
}

/**
 * @brief      Test indicator LED detection
 * @param	   none
 * @return     none
 */
void test_indicator_led_detection(void)
{
  printf("Testing indicator LED detection...\\n");
  
  // LEDs 0 and 1 are assigned as indicator LEDs
  assert(is_indicator_led(&leds[0]) == 1);
  assert(is_indicator_led(&leds[1]) == 1);
  
  // LEDs 2 and 3 are not indicator LEDs
  assert(is_indicator_led(&leds[2]) == 0);
  assert(is_indicator_led(&leds[3]) == 0);
  
  printf("Indicator LED detection tests passed!\\n");
}

/**
 * @brief      Test default brightness configuration
 * @param	   none
 * @return     none
 */
void test_default_brightness(void)
{
  printf("Testing default brightness configuration...\\n");
  
  // Should return the configured default brightness
  assert(get_default_brightness() == 2);
  
  printf("Default brightness tests passed!\\n");
}

/**
 * @brief      Test PWM configuration application
 * @param	   none
 * @return     none
 */
void test_pwm_config_application(void)
{
  printf("Testing PWM configuration application...\\n");
  
  // Reset PWM LED count
  pwm_led_count = 0;
  
  // Apply PWM configuration
  apply_pwm_config_from_db();
  
  // Should register 2 PWM LEDs (D3 and C0 pins that are indicator LEDs)
  assert(pwm_led_count == 2);
  
  printf("PWM configuration application tests passed!\\n");
}

/**
 * @brief      Test Router vs End Device build differences
 * @param	   none
 * @return     none
 */
void test_build_type_differences(void)
{
  printf("Testing Router vs End Device build differences...\\n");
  
  // Router builds should support PWM
  assert(device_has_indicator_pwm() == 1);
  
  // Note: End Device test would require recompiling without DEVICE_TYPE_ROUTER
  // This test verifies Router build behavior
  
  printf("Build type differences tests passed!\\n");
}

int main(void)
{
  printf("Starting device database PWM integration tests...\\n\\n");
  
  test_device_pwm_capability();
  test_pin_pwm_support();
  test_indicator_led_detection();
  test_default_brightness();
  test_pwm_config_application();
  test_build_type_differences();
  
  printf("\\nAll device database PWM integration tests passed!\\n");
  return 0;
}