#ifndef _DIMMER_KEY_CLUSTER_H_
#define _DIMMER_KEY_CLUSTER_H_

#include "base_components/button.h"
#include "hal/zigbee.h"
#include <stdint.h>

typedef struct {
    uint16_t button_long_press_duration;
    uint8_t  level_move_rate;
} zigbee_dimmer_key_cluster_config;

typedef struct {
    // Parameters
    uint8_t              dimmer_key_idx;
    uint8_t              endpoint;
    button_t *           up_button;
    button_t *           down_button;

    // Attributes
    uint8_t              level_move_rate;
    hal_zigbee_attribute config_attr_infos[2];

    uint16_t             present_value;
    hal_zigbee_attribute multistate_attr_infos[4];
} zigbee_dimmer_key_cluster;

void dimmer_key_cluster_add_to_endpoint(zigbee_dimmer_key_cluster *cluster,
                                        hal_zigbee_endpoint *endpoint);

void dimmer_key_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                       uint16_t attribute_id);

#endif
