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
