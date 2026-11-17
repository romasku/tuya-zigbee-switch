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
    hal_zigbee_attribute attr_infos[4];             // onoff, startup, current_level, on_level
    hal_zigbee_attribute ballast_attr_infos[2];     // min/max level (lightingBallastCfg)
    hal_zigbee_attribute switch_type_attr_infos[1]; // switch type (genOnOffSwitchCfg)
    uint8_t              current_level;
    uint8_t              min_level;
    uint8_t              max_level;
    uint8_t              switch_type;
    uint8_t              on;

    /* ZCL OnLevel: the level the light comes on at, from the wall switch or
     * an On command. 0xFF (default) = wherever the MCU left it. */
    uint8_t              on_level;

    /* Set once the MCU has told us its on/off state, so its first report
     * after boot is not taken for a switch press. */
    uint8_t              state_known;

    /* What the MCU uses: its own min/max brightness (Tuya 10..1000, 0 =
     * not known yet) and the last level value written to or reported by it. */
    uint16_t             mcu_min;
    uint16_t             mcu_max;
    uint32_t             level_value;

    /* Spread ZCL levels 1..254 over mcu_min..mcu_max (default). Cleared by
     * the DR config token for the stock behaviour (1..254 -> 10..1000, the
     * MCU clamps, so the ends of the slider do nothing). */
    uint8_t              scale_levels;
    /* Level ramp (ZCL Move / MoveWithOnOff). */
    hal_task_t           ramp_task;
    uint8_t              move_direction; /* 0 = down, 1 = up */
    uint8_t              move_active;
    uint8_t              move_with_onoff;

    /* The MCU fades towards a written level and reports where it is on the
     * way. While a fade we started is running, those reports are kept aside
     * instead of overwriting current_level. */
    hal_task_t           holdoff_task;
    uint8_t              level_holdoff;
    uint8_t              has_held_report;
    uint32_t             held_report_value;
    uint32_t             last_written_value;
} zigbee_dimmer_cluster;

void dimmer_cluster_add_to_endpoint(zigbee_dimmer_cluster *cluster,
                                    hal_zigbee_endpoint *endpoint);


// Returns HAL_ZIGBEE_CMD_SKIPPED-equivalent behavior: no-op if this endpoint
// has no registered dimmer cluster (e.g. the write targeted a relay endpoint
// sharing the same ZCL_CLUSTER_ON_OFF cluster id).
void dimmer_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                   uint16_t attribute_id);

#endif
