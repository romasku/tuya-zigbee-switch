#ifndef _ENCODER_CLUSTER_H_
#define _ENCODER_CLUSTER_H_

#include "base_components/encoder.h"
#include "hal/zigbee.h"
#include <stdint.h>
#include "step_command_handler.h"

typedef struct {
    uint8_t                switch_idx;
    uint8_t                endpoint;

    encoder_t *            encoder;
    step_command_handler_t brightness_step_command_handler;
    step_command_handler_t color_temp_step_command_handler;
} zigbee_encoder_cluster;

void encoder_cluster_add_to_endpoint(zigbee_encoder_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint);

#endif
