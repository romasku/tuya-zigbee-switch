#ifndef _COVER_SWITCH_CLUSTER_H_
#define _COVER_SWITCH_CLUSTER_H_

#include "base_components/button.h"
#include "hal/zigbee.h"
#include <stdint.h>

typedef struct {
    uint8_t cover_index;
    uint8_t reversal;
    uint8_t local_mode;
    uint8_t binded_mode;
    uint8_t switch_type;
} zigbee_cover_switch_cluster_config;

typedef struct {
    // Parameters
    uint8_t              cover_switch_idx;
    uint8_t              endpoint;
    button_t *           open_button;
    button_t *           close_button;

    // Attributes
    uint8_t              switch_type;
    uint8_t              cover_index;
    uint8_t              reversal;
    uint8_t              local_mode;
    uint8_t              binded_mode;
    hal_zigbee_attribute config_attr_infos[6];

    uint16_t             present_value;
    hal_zigbee_attribute multistate_attr_infos[4];

    // State
    uint8_t              binded_moving;
} zigbee_cover_switch_cluster;

void cover_switch_cluster_add_to_endpoint(zigbee_cover_switch_cluster *cluster,
                                          hal_zigbee_endpoint *endpoint);

void cover_switch_cluster_callback_attr_write_trampoline(uint8_t endpoint, uint16_t attribute_id);

#endif
