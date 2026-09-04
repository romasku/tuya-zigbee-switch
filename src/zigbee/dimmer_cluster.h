#ifndef _DIMMER_CLUSTER_H_
#define _DIMMER_CLUSTER_H_

#include "hal/zigbee.h"
#include "hal/tasks.h"

typedef struct {
    uint8_t              dimmer_idx;
    uint8_t              endpoint;
    uint8_t              startup_mode;
    uint8_t              onoff_dpid;
    uint8_t              level_dpid;
    uint8_t              switch_type_dpid;
    uint8_t              min_level_dpid;
    uint8_t              max_level_dpid;
    uint8_t              power_on_behavior_dpid;
    hal_zigbee_attribute attr_infos[3];             // onoff, startup, current_level
    hal_zigbee_attribute ballast_attr_infos[2];     // min/max level (lightingBallastCfg)
    hal_zigbee_attribute switch_type_attr_infos[1]; // switch type (genOnOffSwitchCfg)
    uint8_t              current_level;
    uint8_t              min_level;
    uint8_t              max_level;
    uint8_t              switch_type;
    uint8_t              on;
    /* Wall-switch dimming (ZCL Move/Stop): periodic ramp state. */
    hal_task_t           ramp_task;
    uint8_t              move_direction; /* 0 = down, 1 = up */
    uint8_t              move_active;
} zigbee_dimmer_cluster;

void dimmer_cluster_add_to_endpoint(zigbee_dimmer_cluster *cluster,
                                    hal_zigbee_endpoint *endpoint);

void dimmer_cluster_on(zigbee_dimmer_cluster *cluster);
void dimmer_cluster_off(zigbee_dimmer_cluster *cluster);
void dimmer_cluster_set_level(zigbee_dimmer_cluster *cluster, uint8_t level);

// Returns HAL_ZIGBEE_CMD_SKIPPED-equivalent behavior: no-op if this endpoint
// has no registered dimmer cluster (e.g. the write targeted a relay endpoint
// sharing the same ZCL_CLUSTER_ON_OFF cluster id).
void dimmer_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                   uint16_t attribute_id);

#endif
