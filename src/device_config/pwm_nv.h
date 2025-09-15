#ifndef _PWM_NV_H_
#define _PWM_NV_H_

#include "tl_common.h"

#ifdef INDICATOR_PWM_SUPPORT

typedef struct
{
  u8 pwm_enabled;
  u8 pwm_brightness;
} pwm_nv_config_t;

u8 pwm_nv_read_config(u8 endpoint, pwm_nv_config_t *config);
void pwm_nv_write_config(u8 endpoint, pwm_nv_config_t *config);
void pwm_nv_remove_config(u8 endpoint);

#endif

#endif