
#ifndef _LED_PWM_H_
#define _LED_PWM_H_

#include "types.h"

#ifdef INDICATOR_PWM_SUPPORT

#define PWM_BASE_FREQUENCY_HZ     500
#define PWM_RESOLUTION_STEPS      16
#define MAX_PWM_LEDS              4


typedef struct
{
  u8 led_index;
  u8 brightness_step;
  u8 pin_state;
  u8 enabled;
} led_pwm_channel_t;

typedef struct
{
  led_pwm_channel_t channels[MAX_PWM_LEDS];
  u8 active_channels;
  u8 current_cycle_step;
  u8 timer_enabled;
} led_pwm_manager_t;

typedef struct
{
  u8 led_index;
  u8 default_brightness;
  u8 enabled;
} led_pwm_config_t;

extern led_pwm_config_t pwm_led_registry[MAX_PWM_LEDS];
extern u8 pwm_led_count;

/**
 * @brief      Initialize PWM system
 * @return     none
 */
void led_pwm_init(void);

/**
 * @brief      Enable PWM for LED
 * @param      led_index - LED index
 * @param      brightness_step - Brightness (0-15)
 * @return     none
 */
void led_pwm_enable(u8 led_index, u8 brightness_step);

/**
 * @brief      Disable PWM for LED
 * @param      led_index - LED index
 * @return     none
 */
void led_pwm_disable(u8 led_index);

/**
 * @brief      Set PWM brightness
 * @param      led_index - LED index
 * @param      brightness_step - Brightness (0-15)
 * @return     none
 */
void led_pwm_set_brightness(u8 led_index, u8 brightness_step);

/**
 * @brief      Save PWM state
 * @param      led_index - LED index
 * @return     none
 */
void led_pwm_save_state(u8 led_index);

/**
 * @brief      Restore PWM state
 * @param      led_index - LED index
 * @return     none
 */
void led_pwm_restore_state(u8 led_index);

/**
 * @brief      PWM timer handler
 * @return     none
 */
void led_pwm_timer_handler(void);



/**
 * @brief      Deinitialize PWM system and release resources
 * @param	   none
 * @return     none
 */
void led_pwm_deinit(void);

/**
 * @brief      Register LED for PWM
 * @param      led_index - LED index
 * @param      default_brightness - Default brightness
 * @return     1 if success, 0 if failed
 */
u8 led_pwm_register_led(u8 led_index, u8 default_brightness);

/**
 * @brief      Check if LED is registered for PWM
 * @param      led_index - LED index
 * @return     1 if registered, 0 if not
 */
u8 led_pwm_is_registered(u8 led_index);

/**
 * @brief      Get default brightness for LED
 * @param      led_index - LED index
 * @return     Default brightness (0-15)
 */
u8 led_pwm_get_default_brightness(u8 led_index);



#else
#define led_pwm_init()
#define led_pwm_enable(idx, brightness)
#define led_pwm_disable(idx)
#define led_pwm_set_brightness(idx, brightness)
#define led_pwm_save_state(idx)
#define led_pwm_restore_state(idx)
#define led_pwm_register_led(idx, brightness) 0
#define led_pwm_is_registered(idx) 0
#define led_pwm_get_default_brightness(idx) 0

#define led_pwm_deinit()
#endif

#endif