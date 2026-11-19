#include "zigbee/dimmer_cluster.h"
#include "zigbee/cluster_common.h"
#include "zigbee/consts.h"
#include "zigbee/tuya_mcu_app.h"
#include "zigbee/tuya_secondary_mcu.h"

#define DIMMER_MAX_ENDPOINTS    10
static zigbee_dimmer_cluster *dimmer_cluster_by_endpoint[DIMMER_MAX_ENDPOINTS];

static zigbee_dimmer_cluster *dimmer_by_endpoint(uint8_t endpoint) {
    return endpoint < DIMMER_MAX_ENDPOINTS ? dimmer_cluster_by_endpoint[endpoint]
                                           : NULL;
}

/* The MCU's brightness DPs use 10..1000 (Tuya schema of the DMS16 family:
 * bright_value / brightness_min / brightness_max all declare min 10), ZCL
 * levels use 1..254 for an "on" light. */
#define TUYA_BRIGHTNESS_MIN    10
#define TUYA_BRIGHTNESS_MAX    1000
#define ZCL_LEVEL_MIN          1
#define ZCL_LEVEL_MAX          254

static uint8_t dimmer_clamp_level(int level) {
    if (level < ZCL_LEVEL_MIN)
        return ZCL_LEVEL_MIN;

    if (level > ZCL_LEVEL_MAX)
        return ZCL_LEVEL_MAX;

    return (uint8_t)level;
}

/* Absolute mapping, used for the ballast min/max attributes and when level
 * scaling is off: 1..254 <-> 10..1000. */
static uint32_t dimmer_zcl_level_to_tuya_value(uint8_t zcl_level) {
    uint32_t value = ((uint32_t)dimmer_clamp_level(zcl_level) * TUYA_BRIGHTNESS_MAX +
                      ZCL_LEVEL_MAX / 2) / ZCL_LEVEL_MAX;

    return value < TUYA_BRIGHTNESS_MIN ? TUYA_BRIGHTNESS_MIN : value;
}

static uint8_t dimmer_tuya_value_to_zcl_level(uint32_t tuya_value) {
    if (tuya_value > TUYA_BRIGHTNESS_MAX)
        tuya_value = TUYA_BRIGHTNESS_MAX;
    return dimmer_clamp_level((int)((tuya_value * ZCL_LEVEL_MAX +
                                     TUYA_BRIGHTNESS_MAX / 2) / TUYA_BRIGHTNESS_MAX));
}

/* Current level <-> MCU brightness. With scaling on and the MCU's range
 * known, level 1 is the MCU's minimum and 254 its maximum, so the whole
 * slider does something. */
static uint8_t dimmer_scaling_active(const zigbee_dimmer_cluster *cluster) {
    return cluster->scale_levels &&
           cluster->mcu_min >= TUYA_BRIGHTNESS_MIN &&
           cluster->mcu_max <= TUYA_BRIGHTNESS_MAX &&
           cluster->mcu_max > cluster->mcu_min;
}

static uint32_t dimmer_level_to_value(const zigbee_dimmer_cluster *cluster, uint8_t level) {
    if (!dimmer_scaling_active(cluster))
        return dimmer_zcl_level_to_tuya_value(level);

    uint32_t span = (uint32_t)(cluster->mcu_max - cluster->mcu_min);
    uint32_t step = (uint32_t)(dimmer_clamp_level(level) - ZCL_LEVEL_MIN);

    return cluster->mcu_min +
           (step * span + (ZCL_LEVEL_MAX - ZCL_LEVEL_MIN) / 2) / (ZCL_LEVEL_MAX - ZCL_LEVEL_MIN);
}

static uint8_t dimmer_value_to_level(const zigbee_dimmer_cluster *cluster, uint32_t value) {
    if (!dimmer_scaling_active(cluster))
        return dimmer_tuya_value_to_zcl_level(value);

    if (value <= cluster->mcu_min)
        return ZCL_LEVEL_MIN;

    if (value >= cluster->mcu_max)
        return ZCL_LEVEL_MAX;

    uint32_t span = (uint32_t)(cluster->mcu_max - cluster->mcu_min);
    return dimmer_clamp_level(ZCL_LEVEL_MIN +
                              (int)(((value - cluster->mcu_min) * (ZCL_LEVEL_MAX - ZCL_LEVEL_MIN) +
                                     span / 2) / span));
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

static void dimmer_write_tuya_value(uint8_t dpid, uint32_t value) {
    uint8_t bytes[4];

    dimmer_encode_tuya_value(value, bytes);
    tuya_secondary_mcu_write_dp(dpid, TUYA_DP_TYPE_VALUE, bytes, sizeof(bytes));
}

/* ---- local state + notifications ---------------------------------------- */

static void dimmer_notify_onoff(zigbee_dimmer_cluster *cluster) {
    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_ON_OFF,
                                        ZCL_ATTR_ONOFF);
}

static void dimmer_notify_level(zigbee_dimmer_cluster *cluster) {
    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_LEVEL_CONTROL,
                                        ZCL_ATTR_LEVEL_CURRENT_LEVEL);
}

static void dimmer_set_current_level(zigbee_dimmer_cluster *cluster, uint8_t level) {
    if (cluster->current_level == level)
        return;

    cluster->current_level = level;
    dimmer_notify_level(cluster);
}

static void dimmer_set_on(zigbee_dimmer_cluster *cluster, uint8_t on) {
    on = on ? 1 : 0;
    if (cluster->on == on)
        return;

    cluster->on = on;
    dimmer_notify_onoff(cluster);
}

/* ---- MCU writes ---------------------------------------------------------- */

/* How long reports of the level DP are treated as the MCU's own fade towards
 * a level we wrote. The MCU always fades (both the WiFi and Zigbee DMS16
 * variants); a fade takes about 1 s, 3 s leaves margin. */
#define DIMMER_FADE_HOLDOFF_MS    3000

static void dimmer_start_holdoff(zigbee_dimmer_cluster *cluster) {
    cluster->level_holdoff   = 1;
    cluster->has_held_report = 0;
    hal_tasks_unschedule(&cluster->holdoff_task);
    hal_tasks_schedule(&cluster->holdoff_task, DIMMER_FADE_HOLDOFF_MS);
}

static void dimmer_holdoff_done(void *arg) {
    zigbee_dimmer_cluster *cluster = (zigbee_dimmer_cluster *)arg;

    cluster->level_holdoff = 0;
    if (!cluster->has_held_report)
        return;

    cluster->has_held_report = 0;

    /* The fade has settled somewhere else than where we sent it (e.g. the MCU
     * clamped to its own min/max, or the wall switch was used meanwhile):
     * the MCU is the truth. */
    if (cluster->held_report_value != cluster->last_written_value) {
        cluster->level_value = cluster->held_report_value;
        dimmer_set_current_level(cluster,
                                 dimmer_value_to_level(cluster, cluster->held_report_value));
    }
}

static void dimmer_write_level(zigbee_dimmer_cluster *cluster, uint8_t level) {
    uint32_t value = dimmer_level_to_value(cluster, level);

    dimmer_write_tuya_value(cluster->level_dpid, value);
    cluster->last_written_value = value;
    cluster->level_value        = value;
    dimmer_start_holdoff(cluster);
}

static void dimmer_write_onoff(zigbee_dimmer_cluster *cluster, uint8_t on) {
    uint8_t value = on ? 1 : 0;

    tuya_secondary_mcu_write_dp(cluster->onoff_dpid, TUYA_DP_TYPE_BOOL,
                                &value, sizeof(value));
}

static void dimmer_stop_move(zigbee_dimmer_cluster *cluster) {
    cluster->move_active = 0;
    hal_tasks_unschedule(&cluster->ramp_task);
}

/* On before level: the MCU acknowledges a level written while the channel is
 * off but does not keep it, and "on" restores the level it remembers.
 * So "on at level X" is on first, then X. */
static void dimmer_turn_on(zigbee_dimmer_cluster *cluster) {
    dimmer_write_onoff(cluster, 1);
    dimmer_set_on(cluster, 1);
}

static void dimmer_turn_on_at(zigbee_dimmer_cluster *cluster, uint8_t level) {
    dimmer_turn_on(cluster);
    dimmer_set_current_level(cluster, level);
    dimmer_write_level(cluster, level);
}

static uint8_t dimmer_on_level_set(const zigbee_dimmer_cluster *cluster) {
    return cluster->on_level >= ZCL_LEVEL_MIN && cluster->on_level <= ZCL_LEVEL_MAX;
}

/* ZCL On: to OnLevel when one is set, else wherever the MCU left it. */
static void dimmer_turn_on_default(zigbee_dimmer_cluster *cluster) {
    if (dimmer_on_level_set(cluster))
        dimmer_turn_on_at(cluster, cluster->on_level);
    else
        dimmer_turn_on(cluster);
}

static void dimmer_turn_off(zigbee_dimmer_cluster *cluster) {
    dimmer_stop_move(cluster);
    dimmer_write_onoff(cluster, 0);
    dimmer_set_on(cluster, 0);
}

/* Level change without OnOff: no effect while off (ZCL, and the Telink stack
 * drops MoveToLevel for an off light before it reaches us anyway). */
static void dimmer_change_level(zigbee_dimmer_cluster *cluster, uint8_t level) {
    if (!cluster->on)
        return;

    dimmer_set_current_level(cluster, level);
    dimmer_write_level(cluster, level);
}

/* ---- ramp (Move / MoveWithOnOff) ----------------------------------------- */

#define DIMMER_RAMP_PERIOD_MS    300
#define DIMMER_RAMP_STEP         12  /* ~40 levels/s, same as the wall switch */

static void dimmer_cluster_ramp_step(void *arg) {
    zigbee_dimmer_cluster *cluster = (zigbee_dimmer_cluster *)arg;

    if (cluster == NULL || !cluster->move_active)
        return;

    int next = cluster->current_level +
               (cluster->move_direction ? DIMMER_RAMP_STEP : -DIMMER_RAMP_STEP);
    uint8_t level    = dimmer_clamp_level(next);
    uint8_t at_limit = cluster->move_direction ? level >= ZCL_LEVEL_MAX
                                               : level <= ZCL_LEVEL_MIN;

    if (level != cluster->current_level) {
        dimmer_set_current_level(cluster, level);
        dimmer_write_level(cluster, level);
    }

    if (at_limit) {
        cluster->move_active = 0;
        /* ZCL: MoveWithOnOff down to the minimum switches the light off. */
        if (!cluster->move_direction && cluster->move_with_onoff)
            dimmer_turn_off(cluster);
        return;
    }
    hal_tasks_unschedule(&cluster->ramp_task);
    hal_tasks_schedule(&cluster->ramp_task, DIMMER_RAMP_PERIOD_MS);
}

/* ---- ZCL command handlers ------------------------------------------------ */

static hal_zigbee_cmd_result_t dimmer_cluster_callback(zigbee_dimmer_cluster *cluster,
                                                       uint8_t command_id,
                                                       void *cmd_payload,
                                                       uint16_t cmd_payload_len) {
    if (cluster == NULL)
        return HAL_ZIGBEE_CMD_SKIPPED;

    switch (command_id) {
    case ZCL_CMD_ONOFF_ON:
    case ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE:
        dimmer_turn_on_default(cluster);
        break;
    case ZCL_CMD_ONOFF_OFF:
    case ZCL_CMD_OFF_WITH_EFFECT:
        dimmer_turn_off(cluster);
        break;
    case ZCL_CMD_ONOFF_TOGGLE:
        if (cluster->on)
            dimmer_turn_off(cluster);
        else
            dimmer_turn_on_default(cluster);
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
    const uint8_t *payload = (const uint8_t *)cmd_payload;

    if (cluster == NULL)
        return HAL_ZIGBEE_CMD_SKIPPED;

    switch (command_id) {
    case ZCL_CMD_LEVEL_MOVE_TO_LEVEL:
    case ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF: {
        /* Payload: level, transition time (u16), [options mask, override] */
        if (payload == NULL || cmd_payload_len < 1)
            return HAL_ZIGBEE_MALFORMED_COMMAND;

        uint8_t with_onoff = command_id == ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF;
        dimmer_stop_move(cluster);

        if (!with_onoff) {
            dimmer_change_level(cluster, dimmer_clamp_level(payload[0]));
            break;
        }
        if (payload[0] == 0) {
            /* Level 0 with OnOff = off; keep the level for the next "on". */
            dimmer_turn_off(cluster);
            break;
        }

        /* Home Assistant turns dimmers on with this command only, and the
         * MCU does not switch on from a level: on, then the level. */
        dimmer_turn_on_at(cluster, dimmer_clamp_level(payload[0]));
        break;
    }

    case ZCL_CMD_LEVEL_MOVE:
    case ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF: {
        /* Payload: move mode, rate, [options mask, override]. The rate is not
         * used: the ramp runs at the wall switch's fixed speed. */
        if (payload == NULL || cmd_payload_len < 2)
            return HAL_ZIGBEE_MALFORMED_COMMAND;

        uint8_t with_onoff = command_id == ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF;
        uint8_t up         = payload[0] == ZCL_LEVEL_MOVE_UP;

        if (!cluster->on) {
            if (!(with_onoff && up))
                break; /* nothing to move while off */
            /* ZCL: start from the minimum level when switching on by moving. */
            dimmer_turn_on_at(cluster, ZCL_LEVEL_MIN);
        }
        cluster->move_direction  = up;
        cluster->move_with_onoff = with_onoff;
        cluster->move_active     = 1;
        hal_tasks_unschedule(&cluster->ramp_task);
        hal_tasks_schedule(&cluster->ramp_task, DIMMER_RAMP_PERIOD_MS);
        break;
    }

    case ZCL_CMD_LEVEL_STEP:
    case ZCL_CMD_LEVEL_STEP_WITH_ON_OFF: {
        /* Payload: step mode, step size, transition time (u16), [options] */
        if (payload == NULL || cmd_payload_len < 2)
            return HAL_ZIGBEE_MALFORMED_COMMAND;

        uint8_t with_onoff = command_id == ZCL_CMD_LEVEL_STEP_WITH_ON_OFF;
        uint8_t up         = payload[0] == ZCL_LEVEL_MOVE_UP;
        int     base       = cluster->current_level;

        dimmer_stop_move(cluster);
        if (!cluster->on) {
            if (!(with_onoff && up))
                break;
            base = 0; /* stepping up from off starts at the bottom */
        }

        int next = up ? base + payload[1] : base - payload[1];
        if (!up && next < ZCL_LEVEL_MIN && with_onoff) {
            dimmer_turn_off(cluster);
            break;
        }
        uint8_t level = dimmer_clamp_level(next);
        if (!cluster->on) {
            dimmer_turn_on_at(cluster, level);
        } else {
            dimmer_set_current_level(cluster, level);
            dimmer_write_level(cluster, level);
        }
        break;
    }

    case ZCL_CMD_LEVEL_STOP:
    case ZCL_CMD_LEVEL_STOP_WITH_ON_OFF:
        dimmer_stop_move(cluster);
        break;

    default:
        return HAL_ZIGBEE_CMD_SKIPPED;
    }
    return HAL_ZIGBEE_CMD_PROCESSED;
}

/* ---- MCU reports --------------------------------------------------------- */

/* The MCU's min or max changed: the same light output is another level now. */
static void dimmer_range_changed(zigbee_dimmer_cluster *cluster) {
    if (cluster->level_value != 0)
        dimmer_set_current_level(cluster, dimmer_value_to_level(cluster, cluster->level_value));
}

static void dimmer_cluster_on_dp_report(uint8_t dpid, uint8_t dp_type,
                                        const uint8_t *value, uint16_t value_len) {
    if (value_len < 1)
        return;

    // A single DP report can apply to any registered dimmer endpoint, since
    // DPIDs are assigned per-dimmer, not globally.
    for (int i = 0; i < DIMMER_MAX_ENDPOINTS; i++) {
        zigbee_dimmer_cluster *cluster = dimmer_cluster_by_endpoint[i];
        if (cluster == NULL)
            continue;

        uint32_t number = dimmer_decode_tuya_value(value, value_len);

        if (dp_type == TUYA_DP_TYPE_BOOL && dpid == cluster->onoff_dpid) {
            /* Our own "on" has already set cluster->on, so only the MCU
             * switching on by itself (the wall switch) gets here while off.
             * Not an answer to our query, not the first report after boot. */
            uint8_t switched_on = value[0] && cluster->state_known && !cluster->on &&
                                  !tuya_secondary_mcu_report_is_passive();

            if (!value[0])
                dimmer_stop_move(cluster);
            dimmer_set_on(cluster, value[0]);
            cluster->state_known = 1;

            /* Go to OnLevel at once, with no round trip through the
             * network: the MCU is fading in to the level it remembers and
             * turns towards this one instead. */
            if (switched_on && dimmer_on_level_set(cluster)) {
                dimmer_set_current_level(cluster, cluster->on_level);
                dimmer_write_level(cluster, cluster->on_level);
            }
        } else if (dp_type == TUYA_DP_TYPE_VALUE && dpid == cluster->level_dpid) {
            if (cluster->move_active || cluster->level_holdoff) {
                /* Our own ramp or fade in progress: the MCU reports where
                 * its smoothing is, not where the light is going. */
                cluster->held_report_value = number;
                cluster->has_held_report   = 1;
            } else {
                cluster->level_value = number;
                dimmer_set_current_level(cluster, dimmer_value_to_level(cluster, number));
            }
        } else if (dp_type == TUYA_DP_TYPE_VALUE && cluster->min_level_dpid &&
                   dpid == cluster->min_level_dpid) {
            cluster->mcu_min   = (uint16_t)(number > 0xFFFF ? 0xFFFF : number);
            cluster->min_level = dimmer_tuya_value_to_zcl_level(number);
            hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                                ZCL_CLUSTER_LIGHTING_BALLAST_CONFIG,
                                                ZCL_ATTR_BALLAST_MIN_LEVEL);
            dimmer_range_changed(cluster);
        } else if (dp_type == TUYA_DP_TYPE_VALUE && cluster->max_level_dpid &&
                   dpid == cluster->max_level_dpid) {
            cluster->mcu_max   = (uint16_t)(number > 0xFFFF ? 0xFFFF : number);
            cluster->max_level = dimmer_tuya_value_to_zcl_level(number);
            hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                                ZCL_CLUSTER_LIGHTING_BALLAST_CONFIG,
                                                ZCL_ATTR_BALLAST_MAX_LEVEL);
            dimmer_range_changed(cluster);
        } else if (dp_type == TUYA_DP_TYPE_ENUM && cluster->switch_type_dpid &&
                   dpid == cluster->switch_type_dpid) {
            cluster->switch_type = value[0];
            hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                                ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
                                                ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE);
        } else if (dp_type == TUYA_DP_TYPE_ENUM && cluster->power_on_behavior_dpid &&
                   dpid == cluster->power_on_behavior_dpid) {
            cluster->startup_mode = tuya_mcu_startup_from_power_on(value[0]);
            hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_ON_OFF,
                                                ZCL_ATTR_START_UP_ONOFF);
        }
    }
}

/* ---- trampolines + registration ------------------------------------------ */

static hal_zigbee_cmd_result_t dimmer_cluster_callback_trampoline(
    uint8_t endpoint, uint16_t cluster_id, uint8_t command_id,
    void *cmd_payload, uint16_t cmd_payload_len) {
    return dimmer_cluster_callback(dimmer_by_endpoint(endpoint),
                                   command_id, cmd_payload, cmd_payload_len);
}

static hal_zigbee_cmd_result_t dimmer_cluster_level_callback_trampoline(
    uint8_t endpoint, uint16_t cluster_id, uint8_t command_id,
    void *cmd_payload, uint16_t cmd_payload_len) {
    return dimmer_cluster_level_callback(dimmer_by_endpoint(endpoint),
                                         command_id, cmd_payload,
                                         cmd_payload_len);
}

void dimmer_cluster_add_to_endpoint(zigbee_dimmer_cluster *cluster,
                                    hal_zigbee_endpoint *endpoint) {
    cluster->endpoint = endpoint->endpoint;
    if (endpoint->endpoint < DIMMER_MAX_ENDPOINTS)
        dimmer_cluster_by_endpoint[endpoint->endpoint] = cluster;

    cluster->ramp_task.handler = dimmer_cluster_ramp_step;
    cluster->ramp_task.arg     = cluster;
    hal_tasks_init(&cluster->ramp_task);
    cluster->move_active = 0;

    cluster->holdoff_task.handler = dimmer_holdoff_done;
    cluster->holdoff_task.arg     = cluster;
    hal_tasks_init(&cluster->holdoff_task);
    cluster->level_holdoff   = 0;
    cluster->has_held_report = 0;
    cluster->mcu_min         = 0;
    cluster->mcu_max         = 0;
    cluster->level_value     = 0;
    cluster->on_level        = 0xFF;
    cluster->state_known     = 0;

    SETUP_ATTR(0, ZCL_ATTR_ONOFF, ZCL_DATA_TYPE_BOOLEAN, ATTR_READONLY, cluster->on);
    SETUP_ATTR(1, ZCL_ATTR_START_UP_ONOFF, ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE,
               cluster->startup_mode);
    SETUP_ATTR(2, ZCL_ATTR_LEVEL_CURRENT_LEVEL, ZCL_DATA_TYPE_UINT8, ATTR_READONLY,
               cluster->current_level);
    SETUP_ATTR(3, ZCL_ATTR_LEVEL_ON_LEVEL, ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE,
               cluster->on_level);

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
    endpoint->clusters[endpoint->cluster_count].attribute_count = 2;
    endpoint->clusters[endpoint->cluster_count].attributes      = &cluster->attr_infos[2];
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    =
        dimmer_cluster_level_callback_trampoline;
    endpoint->cluster_count++;

    endpoint->clusters[endpoint->cluster_count].cluster_id =
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
    tuya_secondary_mcu_add_dp_listener(dimmer_cluster_on_dp_report);

    /* Nothing is pushed to the MCU here. It stores min/max/switch type and
     * power-on behaviour itself, and those are read back from its answer to
     * query-all at first contact. (A push from here never reached the MCU
     * anyway: the UART link is enabled only after parse_config().) */
}

void dimmer_cluster_on_write_attr(zigbee_dimmer_cluster *cluster,
                                  uint16_t attribute_id) {
    if (cluster == NULL)
        return;

    if (attribute_id == ZCL_ATTR_START_UP_ONOFF && cluster->power_on_behavior_dpid) {
        uint8_t value = tuya_mcu_power_on_from_startup(cluster->startup_mode);
        tuya_secondary_mcu_write_dp(cluster->power_on_behavior_dpid,
                                    TUYA_DP_TYPE_ENUM, &value, sizeof(value));
    } else if (attribute_id == ZCL_ATTR_BALLAST_MIN_LEVEL) {
        if (!cluster->min_level_dpid)
            return;

        dimmer_write_tuya_value(cluster->min_level_dpid,
                                dimmer_zcl_level_to_tuya_value(cluster->min_level));
    } else if (attribute_id == ZCL_ATTR_BALLAST_MAX_LEVEL) {
        if (!cluster->max_level_dpid)
            return;

        dimmer_write_tuya_value(cluster->max_level_dpid,
                                dimmer_zcl_level_to_tuya_value(cluster->max_level));
    } else if (attribute_id == ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE) {
        if (!cluster->switch_type_dpid)
            return;

        tuya_secondary_mcu_write_dp(cluster->switch_type_dpid, TUYA_DP_TYPE_ENUM,
                                    &cluster->switch_type, sizeof(cluster->switch_type));
    }
}

void dimmer_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                   uint16_t attribute_id) {
    dimmer_cluster_on_write_attr(dimmer_by_endpoint(endpoint), attribute_id);
}
