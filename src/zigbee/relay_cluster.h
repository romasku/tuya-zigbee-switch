#ifndef _RELAY_CLUSTER_H_
#define _RELAY_CLUSTER_H_

#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"

#include "endpoint.h"
#include "base_components/relay.h"
#include "base_components/led.h"
#include "base_components/led_pwm.h"
#include "device_config/pwm_nv.h"

typedef struct
{
  u8            relay_idx;
  u8            endpoint;
  u8            startup_mode;
  u8            indicator_led_mode;
#ifdef INDICATOR_PWM_SUPPORT
  zclAttrInfo_t attr_infos[6];
#else
  zclAttrInfo_t attr_infos[4];
#endif
  relay_t *     relay;
  led_t *       indicator_led;
#ifdef INDICATOR_PWM_SUPPORT
  u8            pwm_enabled;
  u8            pwm_brightness;
  u8            pwm_saved_state;
#endif
} zigbee_relay_cluster;

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster, zigbee_endpoint *endpoint);

void relay_cluster_on(zigbee_relay_cluster *cluster);
void relay_cluster_off(zigbee_relay_cluster *cluster);
void relay_cluster_toggle(zigbee_relay_cluster *cluster);

void relay_cluster_report(zigbee_relay_cluster *cluster);

void update_relay_clusters();

void relay_cluster_callback_attr_write_trampoline(u8 clusterId, zclWriteCmd_t *pWriteReqCmd);

void relay_cluster_led_blink(zigbee_relay_cluster *cluster, u16 on_time_ms, u16 off_time_ms, u16 times);

#ifdef INDICATOR_PWM_SUPPORT
void relay_cluster_set_pwm_brightness(zigbee_relay_cluster *cluster, u8 brightness);
u8 relay_cluster_get_pwm_brightness(zigbee_relay_cluster *cluster);
void relay_cluster_enable_pwm(zigbee_relay_cluster *cluster, u8 enable);
u8 relay_cluster_is_pwm_enabled(zigbee_relay_cluster *cluster);
void relay_cluster_save_pwm_state_for_blink(zigbee_relay_cluster *cluster);
void relay_cluster_restore_pwm_state_after_blink(zigbee_relay_cluster *cluster);
#endif

#endif
