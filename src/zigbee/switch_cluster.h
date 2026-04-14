#ifndef _SWITCH_CLUSTER_H_
#define _SWITCH_CLUSTER_H_

#include "base_components/button.h"
#include "base_components/led.h"
#include "hal/tasks.h"
#include "hal/zigbee.h"
#include <stdint.h>

typedef struct {
    uint8_t  mode;
    uint8_t  action;
    uint8_t  relay_mode;
    uint8_t  relay_index;
    uint16_t button_long_press_duration;
    uint8_t  level_move_rate;
    uint8_t  binded_mode;
    // v2 fields (added at end for NVM backward compat)
    uint16_t confirm_release_ms;
    uint8_t  max_press_count;
} zigbee_switch_cluster_config;

typedef struct {
    uint8_t              switch_idx;
    uint8_t              endpoint;
    uint8_t              mode;
    uint8_t              action;
    uint8_t              relay_mode;
    uint8_t              relay_index;
    uint8_t              binded_mode;
    button_t *           button;
    hal_zigbee_attribute attr_infos[10];
    uint16_t             multistate_state;
    uint16_t             multistate_num_of_states;
    hal_zigbee_attribute multistate_attr_infos[4];
    uint8_t              level_move_rate;
    uint8_t              level_move_direction;
    // v2 runtime fields
    uint8_t              n_press;
    uint8_t              in_hold;
    uint16_t             confirm_release_ms;
    uint8_t              max_press_count;
    hal_task_t           timer_hold;
    hal_task_t           timer_confirm;
    led_t *              indicator_led; // LED to flash on frame send (P+I pattern)
} zigbee_switch_cluster;

void switch_cluster_add_to_endpoint(zigbee_switch_cluster *cluster,
                                    hal_zigbee_endpoint *endpoint);

void switch_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                   uint16_t attribute_id);

void update_switch_clusters(void);

#endif
