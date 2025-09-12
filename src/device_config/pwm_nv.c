#include "zcl_include.h"
#include "configs/nv_slots_cfg.h"
#include "base_components/led_pwm.h"
#include "pwm_nv.h"

#ifdef INDICATOR_PWM_SUPPORT

#define NV_ITEM_ZCL_PWM_CONFIG(endpoint) (NV_ITEM_ZCL_PWM_CONFIG_BASE + endpoint)

u8 pwm_nv_read_config(u8 endpoint, pwm_nv_config_t *config)
{
  if (config == NULL || endpoint == 0)
  {
    return 0;
  }

  nv_sts_t st = nv_flashReadNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_PWM_CONFIG(endpoint), 
                                sizeof(pwm_nv_config_t), (u8 *)config);

  if (st != NV_SUCC)
  {
    config->pwm_enabled = 0;
    config->pwm_brightness = 2;
    return 0;
  }

  if (config->pwm_brightness > 15)
  {
    config->pwm_brightness = 2;
  }

  return 1;
}

void pwm_nv_write_config(u8 endpoint, pwm_nv_config_t *config)
{
  if (config == NULL || endpoint == 0)
  {
    return;
  }

  if (config->pwm_brightness > 15)
  {
    config->pwm_brightness = 15;
  }

  nv_sts_t st = nv_flashWriteNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_PWM_CONFIG(endpoint),
                                 sizeof(pwm_nv_config_t), (u8 *)config);

  if (st != NV_SUCC)
  {
    printf("Failed to write PWM config to NV for endpoint %d, st: %d\r\n", endpoint, st);
  }
}

void pwm_nv_remove_config(u8 endpoint)
{
  if (endpoint == 0)
  {
    return;
  }

  nv_flashSingleItemRemove(NV_MODULE_ZCL, NV_ITEM_ZCL_PWM_CONFIG(endpoint), sizeof(pwm_nv_config_t));
}

#endif