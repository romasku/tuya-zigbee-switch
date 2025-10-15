#ifndef _SWITCH_CLUSTER_H_
#define _SWITCH_CLUSTER_H_

#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"

#include "endpoint.h"
#include "base_components/button.h"
#include "custom_zcl/zcl_onoff_configuration.h"

typedef struct
{
  u8  mode;
  u8  action;
  u8  relay_mode;
  u8  relay_index;
  u16 button_long_press_duration;
  u8  level_move_rate;
  u8  binded_mode;

  u8 out_scene_start_offset;
  u8 out_scene_count; // 0 disables it

  u16 out_scene_group_id;
  u16 scene_recall_time;
} zigbee_switch_cluster_config;

#define SWITCH_CLUSTER_ONOFF_CONFIGURATION_ATTRS 14
typedef struct
{
  u8            switch_idx;
  u8            endpoint;
  u8            mode;
  u8            action;
  u8            relay_mode;
  u8            relay_index;
  u8            binded_mode;

  u8            out_scene_start_offset;
  u8            out_scene_count; // 0 disables it
  u8            out_scene_next;

  u16           out_scene_group_id;
  u16           scene_recall_time;

  button_t *    button;
  zclAttrInfo_t attr_infos[SWITCH_CLUSTER_ONOFF_CONFIGURATION_ATTRS];
  u16           multistate_state;
  zclAttrInfo_t multistate_attr_infos[4];
  move_t *      level_move;
} zigbee_switch_cluster;

void switch_cluster_add_to_endpoint(zigbee_switch_cluster *cluster, zigbee_endpoint *endpoint);

void switch_cluster_callback_attr_write_trampoline(u8 endpoint, u16 clusterId, zclWriteCmd_t *pWriteReqCmd);

#endif
