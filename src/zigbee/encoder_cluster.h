#ifndef _ENCODER_CLUSTER_H_
#define _ENCODER_CLUSTER_H_

#include "base_components/encoder.h"
#include "hal/zigbee.h"
#include <stdint.h>
#include "step_command_handler.h"

typedef struct
{
  uint8_t mode;
  uint8_t action;
  uint8_t relay_mode;
  uint8_t relay_index;
  uint16_t button_long_press_duration;
  uint8_t level_move_rate;
  uint8_t binded_mode;
} zigbee_encoder_cluster_config;

typedef struct
{
  uint8_t switch_idx;
  uint8_t endpoint;
  uint8_t mode;
  uint8_t action;
  uint8_t relay_mode;
  uint8_t relay_index;
  uint8_t binded_mode;
  encoder_t *encoder;
  hal_zigbee_attribute attr_infos[8];
  uint16_t multistate_state;
  hal_zigbee_attribute multistate_attr_infos[4];
  uint8_t level_move_rate;
  uint8_t level_move_direction;

  step_command_handler_t brightness_step_command_handler;
} zigbee_encoder_cluster;

void encoder_cluster_add_to_endpoint(zigbee_encoder_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint);

#endif
