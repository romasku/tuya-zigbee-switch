#include "led_pwm.h"

#ifdef INDICATOR_PWM_SUPPORT

#include "tl_common.h"
#include "led.h"
#include <string.h>

led_pwm_config_t pwm_led_registry[MAX_PWM_LEDS];
u8 pwm_led_count = 0;

static led_pwm_manager_t pwm_manager;
static u8 pwm_saved_states[MAX_PWM_LEDS] = {0};

static ev_timer_event_t *pwm_timer_evt = NULL;

extern led_t leds[];
extern u8 leds_cnt;

static void led_pwm_write_pin(u8 led_index, u8 state);
static s8 led_pwm_find_channel(u8 led_index);
static s8 led_pwm_find_free_channel(void);
static void led_pwm_remove_channel(u8 led_index);
static void led_pwm_timer_init(void);
static void led_pwm_timer_deinit(void);

void led_pwm_init(void)
{
  if (pwm_led_count == 0)
  {
    return;
  }

  memset(&pwm_manager, 0, sizeof(pwm_manager));
  pwm_manager.current_cycle_step = 0;
  pwm_manager.timer_enabled = 0;

  led_pwm_timer_init();
}

u8 led_pwm_register_led(u8 led_index, u8 default_brightness)
{
  if (pwm_led_count >= MAX_PWM_LEDS)
  {
    return 0;
  }

  if (led_index >= leds_cnt)
  {
    return 0;
  }

  if (default_brightness > PWM_RESOLUTION_STEPS - 1)
  {
    default_brightness = PWM_RESOLUTION_STEPS - 1;
  }

  for (u8 i = 0; i < pwm_led_count; i++)
  {
    if (pwm_led_registry[i].led_index == led_index)
    {
      pwm_led_registry[i].default_brightness = default_brightness;
      pwm_led_registry[i].enabled = 1;
      return 1;
    }
  }

  pwm_led_registry[pwm_led_count].led_index = led_index;
  pwm_led_registry[pwm_led_count].default_brightness = default_brightness;
  pwm_led_registry[pwm_led_count].enabled = 1;
  pwm_led_count++;

  return 1;
}

u8 led_pwm_is_registered(u8 led_index)
{
  for (u8 i = 0; i < pwm_led_count; i++)
  {
    if (pwm_led_registry[i].led_index == led_index && pwm_led_registry[i].enabled)
    {
      return 1;
    }
  }
  return 0;
}

u8 led_pwm_get_default_brightness(u8 led_index)
{
  for (u8 i = 0; i < pwm_led_count; i++)
  {
    if (pwm_led_registry[i].led_index == led_index && pwm_led_registry[i].enabled)
    {
      return pwm_led_registry[i].default_brightness;
    }
  }
  return 0;
}

void led_pwm_enable(u8 led_index, u8 brightness_step)
{
  if (!led_pwm_is_registered(led_index))
  {
    return;
  }

  if (brightness_step > PWM_RESOLUTION_STEPS - 1)
  {
    brightness_step = PWM_RESOLUTION_STEPS - 1;
  }

  if (brightness_step == 0 || brightness_step == (PWM_RESOLUTION_STEPS - 1))
  {
    led_pwm_disable(led_index);
    if (brightness_step == 0)
    {
      led_off(&leds[led_index]);
    }
    else
    {
      led_on(&leds[led_index]);
    }
    return;
  }

  s8 channel_idx = led_pwm_find_channel(led_index);
  if (channel_idx < 0)
  {
    channel_idx = led_pwm_find_free_channel();
    if (channel_idx < 0)
    {
      return;
    }
    pwm_manager.channels[channel_idx].led_index = led_index;
    pwm_manager.active_channels++;
  }

  pwm_manager.channels[channel_idx].brightness_step = brightness_step;
  pwm_manager.channels[channel_idx].enabled = 1;
  pwm_manager.channels[channel_idx].pin_state = 0;

  if (!pwm_manager.timer_enabled && pwm_manager.active_channels > 0)
  {
    led_pwm_timer_init();
    pwm_manager.timer_enabled = 1;
  }
}

void led_pwm_disable(u8 led_index)
{
  led_pwm_remove_channel(led_index);

  if (pwm_manager.active_channels == 0 && pwm_manager.timer_enabled)
  {
    led_pwm_timer_deinit();
    pwm_manager.timer_enabled = 0;
  }
}

void led_pwm_set_brightness(u8 led_index, u8 brightness_step)
{
  if (brightness_step == 0)
  {
    led_pwm_disable(led_index);
    led_off(&leds[led_index]);
  }
  else if (brightness_step >= (PWM_RESOLUTION_STEPS - 1))
  {
    led_pwm_disable(led_index);
    led_on(&leds[led_index]);
  }
  else
  {
    led_pwm_enable(led_index, brightness_step);
  }
}

void led_pwm_save_state(u8 led_index)
{
  s8 channel_idx = led_pwm_find_channel(led_index);
  if (channel_idx >= 0)
  {
    pwm_saved_states[channel_idx] = pwm_manager.channels[channel_idx].brightness_step;
  }
  else
  {
    pwm_saved_states[led_index] = leds[led_index].on ? (PWM_RESOLUTION_STEPS - 1) : 0;
  }
}

void led_pwm_restore_state(u8 led_index)
{
  s8 channel_idx = led_pwm_find_channel(led_index);
  if (channel_idx >= 0)
  {
    u8 saved_brightness = pwm_saved_states[channel_idx];
    led_pwm_set_brightness(led_index, saved_brightness);
  }
  else
  {
    u8 saved_brightness = pwm_saved_states[led_index];
    led_pwm_set_brightness(led_index, saved_brightness);
  }
}

void led_pwm_timer_handler(void)
{
  pwm_manager.current_cycle_step++;
  if (pwm_manager.current_cycle_step >= PWM_RESOLUTION_STEPS)
  {
    pwm_manager.current_cycle_step = 0;
  }

  for (u8 i = 0; i < MAX_PWM_LEDS; i++)
  {
    if (!pwm_manager.channels[i].enabled)
    {
      continue;
    }

    u8 led_index = pwm_manager.channels[i].led_index;
    u8 brightness = pwm_manager.channels[i].brightness_step;

    u8 should_be_on = (pwm_manager.current_cycle_step < brightness) ? 1 : 0;

    if (pwm_manager.channels[i].pin_state != should_be_on)
    {
      pwm_manager.channels[i].pin_state = should_be_on;
      led_pwm_write_pin(led_index, should_be_on);
    }
  }
}

static void led_pwm_write_pin(u8 led_index, u8 state)
{
  if (led_index >= leds_cnt)
  {
    return;
  }

  led_t *led = &leds[led_index];

  if (state)
  {
    drv_gpio_write(led->pin, led->on_high);
  }
  else
  {
    drv_gpio_write(led->pin, !led->on_high);
  }
}

static s8 led_pwm_find_channel(u8 led_index)
{
  for (u8 i = 0; i < MAX_PWM_LEDS; i++)
  {
    if (pwm_manager.channels[i].enabled && pwm_manager.channels[i].led_index == led_index)
    {
      return i;
    }
  }
  return -1;
}

static s8 led_pwm_find_free_channel(void)
{
  for (u8 i = 0; i < MAX_PWM_LEDS; i++)
  {
    if (!pwm_manager.channels[i].enabled)
    {
      return i;
    }
  }
  return -1;
}

static void led_pwm_remove_channel(u8 led_index)
{
  s8 channel_idx = led_pwm_find_channel(led_index);
  if (channel_idx >= 0)
  {
    pwm_manager.channels[channel_idx].enabled = 0;
    pwm_manager.channels[channel_idx].led_index = 0;
    pwm_manager.channels[channel_idx].brightness_step = 0;
    pwm_manager.channels[channel_idx].pin_state = 0;
    pwm_manager.active_channels--;
  }
}

static int led_pwm_timer_callback(void *arg)
{
  (void)arg;
  led_pwm_timer_handler();
  
  if (pwm_manager.timer_enabled && pwm_manager.active_channels > 0)
  {
    u32 interval_ms = 1000 / (PWM_BASE_FREQUENCY_HZ * PWM_RESOLUTION_STEPS);
    if (interval_ms == 0)
    {
      interval_ms = 1;
    }
    pwm_timer_evt = TL_ZB_TIMER_SCHEDULE(led_pwm_timer_callback, NULL, interval_ms);
  }
  else
  {
    pwm_timer_evt = NULL;
  }
  
  return -1;
}

static void led_pwm_timer_init(void)
{
  if (pwm_timer_evt)
  {
    TL_ZB_TIMER_CANCEL(&pwm_timer_evt);
  }
  
  u32 interval_ms = 1000 / (PWM_BASE_FREQUENCY_HZ * PWM_RESOLUTION_STEPS);
  if (interval_ms == 0)
  {
    interval_ms = 1;
  }
  
  pwm_timer_evt = TL_ZB_TIMER_SCHEDULE(led_pwm_timer_callback, NULL, interval_ms);
}

static void led_pwm_timer_deinit(void)
{
  if (pwm_timer_evt)
  {
    TL_ZB_TIMER_CANCEL(&pwm_timer_evt);
    pwm_timer_evt = NULL;
  }
}





void led_pwm_deinit(void)
{
  if (pwm_led_count == 0)
  {
    return;
  }

  for (u8 i = 0; i < MAX_PWM_LEDS; i++)
  {
    if (pwm_manager.channels[i].enabled)
    {
      led_pwm_disable(pwm_manager.channels[i].led_index);
    }
  }

  if (pwm_manager.timer_enabled)
  {
    led_pwm_timer_deinit();
    pwm_manager.timer_enabled = 0;
  }

  memset(&pwm_manager, 0, sizeof(pwm_manager));
}

#endif // INDICATOR_PWM_SUPPORT