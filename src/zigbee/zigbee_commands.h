#ifndef _ZIGBEE_COMMANDS_H_
#define _ZIGBEE_COMMANDS_H_

#include "consts.h"
#include "hal/zigbee.h"
#include <stddef.h>

static inline hal_zigbee_cmd build_onoff_cmd(uint8_t endpoint,
                                             uint8_t onoff_cmd_id) {
    hal_zigbee_cmd c = {
        .endpoint            = endpoint,
        .profile_id          = ZCL_HA_PROFILE,
        .cluster_id          = ZCL_CLUSTER_ON_OFF,
        .command_id          = onoff_cmd_id,
        .cluster_specific    =                               1,
        .direction           = HAL_ZIGBEE_DIR_CLIENT_TO_SERVER,
        .disable_default_rsp =                               1,
        .manufacturer_code   =                               0,
        .payload             = NULL,
        .payload_len         =                               0,
    };

    return c;
}

static inline hal_zigbee_cmd
build_level_move_onoff_cmd(uint8_t endpoint, uint8_t dir, uint8_t rate) {
    static uint8_t buf[2];

    buf[0] = dir;
    buf[1] = rate;

    hal_zigbee_cmd c = {
        .endpoint            = endpoint,
        .profile_id          = ZCL_HA_PROFILE,
        .cluster_id          = ZCL_CLUSTER_LEVEL_CONTROL,
        .command_id          = ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
        .cluster_specific    =                               1,
        .direction           = HAL_ZIGBEE_DIR_CLIENT_TO_SERVER,
        .disable_default_rsp =                               1,
        .manufacturer_code   =                               0,
        .payload             = buf,
        .payload_len         = sizeof(buf),
    };
    return c;
}

static inline hal_zigbee_cmd build_level_stop_onoff_cmd(uint8_t endpoint) {
    hal_zigbee_cmd c = {
        .endpoint            = endpoint,
        .profile_id          = ZCL_HA_PROFILE,
        .cluster_id          = ZCL_CLUSTER_LEVEL_CONTROL,
        .command_id          = ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
        .cluster_specific    =                               1,
        .direction           = HAL_ZIGBEE_DIR_CLIENT_TO_SERVER,
        .disable_default_rsp =                               1,
        .manufacturer_code   =                               0,
        .payload             = NULL,
        .payload_len         =                               0,
    };

    return c;
}

static inline hal_zigbee_cmd build_level_step_cmd(uint8_t endpoint, uint8_t dir, uint8_t step_size) {
  static uint8_t buf[4];

    buf[0] = dir; // Step Mode 0 up, 1 down
    buf[1] = step_size; // Step size  

    // Transistion Time
    buf[2] = 0; 
    buf[3] = 0;
  
  hal_zigbee_cmd c = {
        .endpoint            = endpoint,
        .profile_id          = ZCL_HA_PROFILE,
        .cluster_id          = ZCL_CLUSTER_LEVEL_CONTROL,
        .command_id          = ZCL_CMD_LEVEL_STEP,
        .cluster_specific    =                               1,
        .direction           = HAL_ZIGBEE_DIR_CLIENT_TO_SERVER,
        .disable_default_rsp =                               1,
        .manufacturer_code   =                               0,
        .payload             = buf,
        .payload_len         = sizeof(buf),
    };

    return c;
}

static inline hal_zigbee_cmd build_color_temp_step_cmd(uint8_t endpoint, uint8_t dir, uint8_t step_size) {
  static uint8_t buf[9];

    buf[0] = dir; // Step Mode 1 up, 3 down

    // Little Endian (least significant byte first)
    // Step size  0x000C - 12
    buf[1] = 0x0C; 
    buf[2] = 0x00;

    // Transistion Time 0x0000 - 0
    buf[3] = 0x00; 
    buf[4] = 0x00;

    // Minimum 0x0000 - 0 - default min
    // TODO: spec says 0x0000 should work as ignore
    buf[5] = 0x00; 
    buf[6] = 0x00;

    // Maximum  0xfeff - 65279 - default max (ffff does not seem to work) 
    // TODO: spec says 0x0000 should work as ignore
    buf[7] = 0xfe; 
    buf[8] = 0xff;
  
  hal_zigbee_cmd c = {
        .endpoint            = endpoint,
        .profile_id          = ZCL_HA_PROFILE,
        .cluster_id          = ZCL_CLUSTER_LIGHTING_COLOR_CONTROL,
        .command_id          = ZCL_CMD_LIGHTING_COLOR_STEP_TEMP,
        .cluster_specific    =                               1,
        .direction           = HAL_ZIGBEE_DIR_CLIENT_TO_SERVER,
        .disable_default_rsp =                               1,
        .manufacturer_code   =                               0,
        .payload             = buf,
        .payload_len         = sizeof(buf),
    };

    return c;
}

static inline hal_zigbee_cmd build_window_covering_cmd(uint8_t endpoint,
                                                       uint8_t cmd_id) {
    hal_zigbee_cmd c = {
        .endpoint            = endpoint,
        .profile_id          = ZCL_HA_PROFILE,
        .cluster_id          = ZCL_CLUSTER_WINDOW_COVERING,
        .command_id          = cmd_id,
        .cluster_specific    =                               1,
        .direction           = HAL_ZIGBEE_DIR_CLIENT_TO_SERVER,
        .disable_default_rsp =                               1,
        .manufacturer_code   =                               0,
        .payload             = NULL,
        .payload_len         =                               0,
    };

    return c;
}

#endif
