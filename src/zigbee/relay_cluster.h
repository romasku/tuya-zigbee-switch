#ifndef _RELAY_CLUSTER_H_
#define _RELAY_CLUSTER_H_

#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"

#include "endpoint.h"
#include "base_components/relay.h"
#include "base_components/led.h"

enum active_effect_next_state {
  NEXT_STATE_OFF,
  NEXT_STATE_ON,
  NEXT_STATE_STOP_EFFECT,
};

enum relay_state {
  RELAY_OFF = 0,
  RELAY_ON = 1,
};

typedef struct zigbee_relay_cluster
{
  u8            relay_idx;
  u8            endpoint;
  u8            startup_mode;
  u8            indicator_led_mode;
  u16           off_wait_time;
  u16           on_wait_time;
  zclAttrInfo_t attr_infos[6];
  relay_t *     relay;
  led_t *       indicator_led;

  u32 on_off_count_from;

  enum active_effect_next_state next_state;
  u32 on_time;
  u32 off_time;
  u32 last_update;
  u32 switch_counter;
  u16 toggles_left;
  
  u16           identify_time;
  zclAttrInfo_t identify_attr_infos[2];

  bool on_off;
  bool indicator_led_on;

  struct zigbee_scene_cluster *scene_cluster;
} zigbee_relay_cluster;

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster, zigbee_endpoint *endpoint);

void relay_cluster_on(zigbee_relay_cluster *cluster);
void relay_cluster_off(zigbee_relay_cluster *cluster);
void relay_cluster_toggle(zigbee_relay_cluster *cluster);

void relay_cluster_report(zigbee_relay_cluster *cluster);

void update_relay_clusters();

void relay_cluster_callback_attr_write_trampoline(u8 clusterId, zclWriteCmd_t *pWriteReqCmd);

void gen_identify_callback_attr_write_trampoline(u8 clusterId, zclWriteCmd_t *pWriteReqCmd);

bool relay_cluster_is_on(zigbee_relay_cluster *cluster);
bool relay_cluster_is_identifying(zigbee_relay_cluster *cluster);

bool relay_cluster_get_on_off(zigbee_relay_cluster *cluster);
void relay_cluster_set_on_off(zigbee_relay_cluster *cluster, bool new_state, bool from_scene);

#endif
