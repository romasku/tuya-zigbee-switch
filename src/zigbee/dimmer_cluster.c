#include "zigbee/dimmer_cluster.h"
#include "zigbee/cluster_common.h"
#include "zigbee/consts.h"
#include "zigbee/tuya_secondary_mcu.h"

static zigbee_dimmer_cluster *dimmer_cluster_by_endpoint[10];

static uint8_t dimmer_default_onoff_dpid(uint8_t endpoint) {
    switch (endpoint) {
    case 1:
        return 0x01;

    case 2:
        return 0x02;

    case 3:
        return 0x03;

    case 4:
        return 0x04;

    default:
        return 0x01;
    }
}

static uint8_t dimmer_default_level_dpid(uint8_t endpoint) {
    switch (endpoint) {
    case 1:
        return 0x08;

    case 2:
        return 0x09;

    case 3:
        return 0x0A;

    case 4:
        return 0x0B;

    default:
        return 0x08;
    }
}

static uint8_t dimmer_get_onoff_dpid(const zigbee_dimmer_cluster *cluster) {
    return cluster->onoff_dpid ? cluster->onoff_dpid
                             : dimmer_default_onoff_dpid(cluster->endpoint);
}

static uint8_t dimmer_get_level_dpid(const zigbee_dimmer_cluster *cluster) {
    return cluster->level_dpid ? cluster->level_dpid
                             : dimmer_default_level_dpid(cluster->endpoint);
}

// The MCU's brightness DP uses a 0-1000 scale (see issue #387), while ZCL
// level control commands use 0-254.
#define TUYA_BRIGHTNESS_MAX    1000
#define ZCL_LEVEL_MAX          254

static uint32_t dimmer_zcl_level_to_tuya_value(uint8_t zcl_level) {
    return ((uint32_t)zcl_level * TUYA_BRIGHTNESS_MAX + ZCL_LEVEL_MAX / 2) /
           ZCL_LEVEL_MAX;
}

static uint8_t dimmer_tuya_value_to_zcl_level(uint32_t tuya_value) {
    if (tuya_value > TUYA_BRIGHTNESS_MAX)
        tuya_value = TUYA_BRIGHTNESS_MAX;
    return (uint8_t)((tuya_value * ZCL_LEVEL_MAX + TUYA_BRIGHTNESS_MAX / 2) /
                     TUYA_BRIGHTNESS_MAX);
}

static void dimmer_encode_tuya_value(uint32_t value, uint8_t out[4]) {
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static uint32_t dimmer_decode_tuya_value(const uint8_t *value, uint16_t value_len) {
    uint32_t result = 0;

    for (uint16_t i = 0; i < value_len && i < 4; i++) {
        result = (result << 8) | value[i];
    }
    return result;
}

void dimmer_cluster_on(zigbee_dimmer_cluster *cluster) {
    cluster->on = 1;
}

void dimmer_cluster_off(zigbee_dimmer_cluster *cluster) {
    cluster->on = 0;
}

void dimmer_cluster_set_level(zigbee_dimmer_cluster *cluster, uint8_t level) {
    /* No software clamp: min/max_level only constrain the hardware wall-button
     * range via the MCU (pushed through min/max DPIDs). Z2M brightness is raw
     * ZCL 0-254 and is not clamped here (matches stock firmware behaviour). */
    cluster->current_level = level;
    cluster->on            = level != 0;
}

static hal_zigbee_cmd_result_t dimmer_cluster_callback_trampoline(
    uint8_t endpoint, uint16_t cluster_id, uint8_t command_id,
    void *cmd_payload, uint16_t cmd_payload_len);

static hal_zigbee_cmd_result_t dimmer_cluster_level_callback_trampoline(
    uint8_t endpoint, uint16_t cluster_id, uint8_t command_id,
    void *cmd_payload, uint16_t cmd_payload_len);

static void dimmer_cluster_on_dp_report(uint8_t dpid, uint8_t dp_type,
                                        const uint8_t *value, uint16_t value_len);

/* Ramp one step for wall-switch dimming (ZCL Move). Runs on a periodic task,
 * stepping current_level toward the move direction and pushing the level DP to
 * the MCU so the light continuously dims/brightens while held. */

#define DIMMER_RAMP_PERIOD_MS   300
#define DIMMER_RAMP_STEP        12   /* Matches hardware: ~12 units per 300ms */

static void dimmer_cluster_ramp_step(void *arg) {
    zigbee_dimmer_cluster *cluster = (zigbee_dimmer_cluster *)arg;
    if (cluster == NULL || !cluster->move_active)
        return;

    uint8_t next = cluster->current_level;
    if (cluster->move_direction) {
        if (next < 254) {
            next = (254 - next < DIMMER_RAMP_STEP) ? 254 : next + DIMMER_RAMP_STEP;
        }
    } else {
        if (next > 0) {
            next = (next < DIMMER_RAMP_STEP) ? 0 : next - DIMMER_RAMP_STEP;
        }
    }

    if (next != cluster->current_level) {
        uint8_t dpid = dimmer_get_level_dpid(cluster);
        uint8_t level_value[4];
        dimmer_encode_tuya_value(dimmer_zcl_level_to_tuya_value(next),
                                 level_value);
        tuya_secondary_mcu_write_dp(dpid, TUYA_DP_TYPE_VALUE,
                                    level_value, sizeof(level_value));
        cluster->current_level = next;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_LEVEL_CONTROL,
                                            ZCL_ATTR_LEVEL_CURRENT_LEVEL);
    }

    if (cluster->move_active) {
        /* Stop at the range limits (0 or 254). */
        if ((cluster->move_direction && next >= 254) || (!cluster->move_direction && next <= 0)) {
            cluster->move_active = 0;
            return;
        }
        hal_tasks_schedule(&cluster->ramp_task, DIMMER_RAMP_PERIOD_MS);
    }
}

static hal_zigbee_cmd_result_t dimmer_cluster_callback(zigbee_dimmer_cluster *cluster,
                                                       uint8_t command_id,
                                                       void *cmd_payload,
                                                       uint16_t cmd_payload_len) {
    switch (command_id) {
    case ZCL_CMD_ONOFF_ON:
    case ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE:
        dimmer_cluster_on(cluster);
        {
            uint8_t dpid = dimmer_get_onoff_dpid(cluster);
            uint8_t onv  = 1;
            tuya_secondary_mcu_write_dp(dpid, TUYA_DP_TYPE_BOOL, &onv, sizeof(onv));
        }
        break;
    case ZCL_CMD_ONOFF_OFF:
    case ZCL_CMD_OFF_WITH_EFFECT:
        dimmer_cluster_off(cluster);
        {
            uint8_t dpid = dimmer_get_onoff_dpid(cluster);
            uint8_t offv = 0;
            tuya_secondary_mcu_write_dp(dpid, TUYA_DP_TYPE_BOOL, &offv, sizeof(offv));
        }
        break;
    case ZCL_CMD_ONOFF_TOGGLE:
        if (cluster->on)
            dimmer_cluster_off(cluster);
        else
            dimmer_cluster_on(cluster);
        {
            uint8_t dpid = dimmer_get_onoff_dpid(cluster);
            uint8_t v    = cluster->on ? 1 : 0;
            tuya_secondary_mcu_write_dp(dpid, TUYA_DP_TYPE_BOOL, &v, sizeof(v));
        }
        break;
    default:
        return HAL_ZIGBEE_CMD_SKIPPED;
    }
    return HAL_ZIGBEE_CMD_PROCESSED;
}

static hal_zigbee_cmd_result_t dimmer_cluster_level_callback(zigbee_dimmer_cluster *cluster,
                                                             uint8_t command_id,
                                                             void *cmd_payload,
                                                             uint16_t cmd_payload_len) {
    switch (command_id) {
    case ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF:
        if (cmd_payload == NULL || cmd_payload_len < 1)
            return HAL_ZIGBEE_MALFORMED_COMMAND;

        {
            uint8_t level = *(uint8_t *)cmd_payload;
            dimmer_cluster_set_level(cluster, level);

            /* Per ZCL, MoveToLevelWithOnOff also changes the OnOff state:
             * on if the target level is > 0, off if it is 0. Home Assistant
             * uses this command to "turn on" a light (it never sends a plain
             * OnOff ON for dimmers), so we MUST also write the on/off DP or
             * the MCU would dim but never switch the relay on. */
            {
                uint8_t dpid   = dimmer_get_onoff_dpid(cluster);
                uint8_t onoff  = (level > 0) ? 1 : 0;
                tuya_secondary_mcu_write_dp(dpid, TUYA_DP_TYPE_BOOL, &onoff,
                                            sizeof(onoff));
            }

            uint8_t dpid = dimmer_get_level_dpid(cluster);
            uint8_t level_value[4];
            dimmer_encode_tuya_value(dimmer_zcl_level_to_tuya_value(level), level_value);
            tuya_secondary_mcu_write_dp(dpid, TUYA_DP_TYPE_VALUE, level_value, sizeof(level_value));
        }
        break;
    case ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF:
        /* Wall-switch dimming: start ramping up/down. Payload = [direction, rate]. */
        if (cmd_payload == NULL || cmd_payload_len < 2)
            return HAL_ZIGBEE_MALFORMED_COMMAND;

        {
            uint8_t direction = ((uint8_t *)cmd_payload)[0];
            cluster->move_direction = (direction == ZCL_LEVEL_MOVE_UP) ? 1 : 0;
            cluster->move_active   = 1;

            /* If it was off and moving up, turn it on first. */
            if (!cluster->on && cluster->move_direction) {
                uint8_t dpid  = dimmer_get_onoff_dpid(cluster);
                uint8_t onoff = 1;
                tuya_secondary_mcu_write_dp(dpid, TUYA_DP_TYPE_BOOL, &onoff,
                                            sizeof(onoff));
                cluster->on = 1;
            }

            hal_tasks_schedule(&cluster->ramp_task, DIMMER_RAMP_PERIOD_MS);
        }
        break;
    case ZCL_CMD_LEVEL_STOP_WITH_ON_OFF:
        /* Wall-switch release: stop ramping. */
        cluster->move_active = 0;
        hal_tasks_unschedule(&cluster->ramp_task);
        break;
    default:
        return HAL_ZIGBEE_CMD_SKIPPED;
    }
    return HAL_ZIGBEE_CMD_PROCESSED;
}

void dimmer_cluster_add_to_endpoint(zigbee_dimmer_cluster *cluster,
                                    hal_zigbee_endpoint *endpoint) {
    cluster->endpoint = endpoint->endpoint;
    dimmer_cluster_by_endpoint[endpoint->endpoint] = cluster;

    cluster->ramp_task.handler = dimmer_cluster_ramp_step;
    cluster->ramp_task.arg     = cluster;
    hal_tasks_init(&cluster->ramp_task);
    cluster->move_active = 0;

    SETUP_ATTR(0, ZCL_ATTR_ONOFF, ZCL_DATA_TYPE_BOOLEAN, ATTR_READONLY, cluster->on);
    SETUP_ATTR(1, ZCL_ATTR_START_UP_ONOFF, ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE,
               cluster->startup_mode);
    SETUP_ATTR(2, ZCL_ATTR_LEVEL_CURRENT_LEVEL, ZCL_DATA_TYPE_UINT8, ATTR_READONLY,
               cluster->current_level);

    // Min/max brightness on the standard lightingBallastCfg cluster (0x0301),
    // so Zigbee2MQTT can render sliders bound to these without a custom cluster.
    SETUP_ATTR_FOR_TABLE(cluster->ballast_attr_infos, 0, ZCL_ATTR_BALLAST_MIN_LEVEL,
                         ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE, cluster->min_level);
    SETUP_ATTR_FOR_TABLE(cluster->ballast_attr_infos, 1, ZCL_ATTR_BALLAST_MAX_LEVEL,
                         ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE, cluster->max_level);

    // Switch type on the standard genOnOffSwitchCfg cluster (0x0007).
    SETUP_ATTR_FOR_TABLE(cluster->switch_type_attr_infos, 0,
                         ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE,
                         ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE, cluster->switch_type);

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_ON_OFF;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 2;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    =
        dimmer_cluster_callback_trampoline;
    endpoint->cluster_count++;

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_LEVEL_CONTROL;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 1;
    endpoint->clusters[endpoint->cluster_count].attributes      = &cluster->attr_infos[2];
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    =
        dimmer_cluster_level_callback_trampoline;
    endpoint->cluster_count++;

    endpoint->clusters[endpoint->cluster_count].cluster_id      =
        ZCL_CLUSTER_LIGHTING_BALLAST_CONFIG;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 2;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->ballast_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    = NULL;
    endpoint->cluster_count++;

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 1;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->switch_type_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    = NULL;
    endpoint->cluster_count++;

    // Receive DP state reports from the secondary MCU (e.g. physical button
    // presses) for every registered dimmer, not just this endpoint.
    tuya_secondary_mcu_register_dp_report_callback(dimmer_cluster_on_dp_report);

    // Push the configured DPIDs to the secondary MCU once at startup, so the
    // parsed Pxx DPID mapping actually takes effect on the hardware side.
    if (cluster->min_level_dpid) {
        uint8_t value_bytes[4];
        dimmer_encode_tuya_value(dimmer_zcl_level_to_tuya_value(cluster->min_level),
                                 value_bytes);
        tuya_secondary_mcu_write_dp(cluster->min_level_dpid, TUYA_DP_TYPE_VALUE,
                                    value_bytes, sizeof(value_bytes));
    }
    if (cluster->max_level_dpid) {
        uint8_t value_bytes[4];
        dimmer_encode_tuya_value(dimmer_zcl_level_to_tuya_value(cluster->max_level),
                                 value_bytes);
        tuya_secondary_mcu_write_dp(cluster->max_level_dpid, TUYA_DP_TYPE_VALUE,
                                    value_bytes, sizeof(value_bytes));
    }
    if (cluster->switch_type_dpid) {
        uint8_t value = cluster->switch_type;
        tuya_secondary_mcu_write_dp(cluster->switch_type_dpid, TUYA_DP_TYPE_ENUM,
                                    &value, sizeof(value));
    }
}

static uint8_t dimmer_power_on_behavior_value(uint8_t startup_mode) {
    // Translate the ZCL StartUpOnOff enum to the common Tuya power-on-behavior
    // DP encoding (0 = off, 1 = on, 2 = memory/previous). Toggle has no direct
    // Tuya equivalent, so it falls back to memory.
    switch (startup_mode) {
    case ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF:
        return 0;

    case ZCL_START_UP_ONOFF_SET_ONOFF_TO_ON:
        return 1;

    default:
        return 2;
    }
}

void dimmer_cluster_on_write_attr(zigbee_dimmer_cluster *cluster,
                                  uint16_t attribute_id) {
    if (cluster == NULL)
        return;

    if (attribute_id == ZCL_ATTR_START_UP_ONOFF && cluster->power_on_behavior_dpid) {
        uint8_t value = dimmer_power_on_behavior_value(cluster->startup_mode);
        tuya_secondary_mcu_write_dp(cluster->power_on_behavior_dpid,
                                    TUYA_DP_TYPE_ENUM, &value, sizeof(value));
    }else if (attribute_id == ZCL_ATTR_BALLAST_MIN_LEVEL) {
        if (!cluster->min_level_dpid)
            return;

        uint8_t value_bytes[4];
        dimmer_encode_tuya_value(dimmer_zcl_level_to_tuya_value(cluster->min_level),
                                 value_bytes);
        tuya_secondary_mcu_write_dp(cluster->min_level_dpid, TUYA_DP_TYPE_VALUE,
                                    value_bytes, sizeof(value_bytes));
    }else if (attribute_id == ZCL_ATTR_BALLAST_MAX_LEVEL) {
        if (!cluster->max_level_dpid)
            return;

        uint8_t value_bytes[4];
        dimmer_encode_tuya_value(dimmer_zcl_level_to_tuya_value(cluster->max_level),
                                 value_bytes);
        tuya_secondary_mcu_write_dp(cluster->max_level_dpid, TUYA_DP_TYPE_VALUE,
                                    value_bytes, sizeof(value_bytes));
    }else if (attribute_id == ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE) {
        if (!cluster->switch_type_dpid)
            return;

        tuya_secondary_mcu_write_dp(cluster->switch_type_dpid, TUYA_DP_TYPE_ENUM,
                                    &cluster->switch_type, sizeof(cluster->switch_type));
    }
}

void dimmer_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                   uint16_t attribute_id) {
    dimmer_cluster_on_write_attr(dimmer_cluster_by_endpoint[endpoint], attribute_id);
}

static void dimmer_cluster_on_dp_report(uint8_t dpid, uint8_t dp_type,
                                        const uint8_t *value, uint16_t value_len) {
    // A single DP report can apply to any registered dimmer endpoint, since
    // DPIDs are assigned per-dimmer, not globally.
    for (int i = 0; i < 10; i++) {
        zigbee_dimmer_cluster *cluster = dimmer_cluster_by_endpoint[i];
        if (cluster == NULL)
            continue;

        if (dp_type == TUYA_DP_TYPE_BOOL && dpid == dimmer_get_onoff_dpid(cluster)) {
            if (value_len < 1)
                continue;
            /* Physical button state change: only update on/off. Brightness is
             * independent and stays at whatever Z2M last had. */
            cluster->on = value[0] ? 1 : 0;
            hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_ON_OFF,
                                                ZCL_ATTR_ONOFF);
        }else if (dp_type == TUYA_DP_TYPE_VALUE && dpid == dimmer_get_level_dpid(cluster)) {
            uint32_t tuya_value = dimmer_decode_tuya_value(value, value_len);
            cluster->current_level = dimmer_tuya_value_to_zcl_level(tuya_value);
            hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_LEVEL_CONTROL,
                                                ZCL_ATTR_LEVEL_CURRENT_LEVEL);
        }
    }
}

static hal_zigbee_cmd_result_t dimmer_cluster_callback_trampoline(
    uint8_t endpoint, uint16_t cluster_id, uint8_t command_id,
    void *cmd_payload, uint16_t cmd_payload_len) {
    return dimmer_cluster_callback(dimmer_cluster_by_endpoint[endpoint],
                                   command_id, cmd_payload, cmd_payload_len);
}

static hal_zigbee_cmd_result_t dimmer_cluster_level_callback_trampoline(
    uint8_t endpoint, uint16_t cluster_id, uint8_t command_id,
    void *cmd_payload, uint16_t cmd_payload_len) {
    return dimmer_cluster_level_callback(dimmer_cluster_by_endpoint[endpoint],
                                         command_id, cmd_payload,
                                         cmd_payload_len);
}
