#include "relay_cluster.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"

hal_zigbee_cmd_result_t relay_cluster_callback(zigbee_relay_cluster *cluster,
                                               uint8_t command_id,
                                               void *cmd_payload,
                                               uint16_t cmd_payload_len);
hal_zigbee_cmd_result_t relay_cluster_callback_trampoline(uint8_t endpoint,
                                                          uint16_t cluster_id,
                                                          uint8_t command_id,
                                                          void *cmd_payload,
                                                          uint16_t cmd_payload_len);

hal_zigbee_cmd_result_t relay_cluster_level_callback(zigbee_relay_cluster *cluster,
                                                     uint8_t command_id,
                                                     void *cmd_payload,
                                                     uint16_t cmd_payload_len);
hal_zigbee_cmd_result_t relay_cluster_level_callback_trampoline(uint8_t endpoint,
                                                                uint16_t cluster_id,
                                                                uint8_t command_id,
                                                                void *cmd_payload,
                                                                uint16_t cmd_payload_len);

void relay_cluster_on_relay_change(zigbee_relay_cluster *cluster,
                                   uint8_t state);
void relay_cluster_on_write_attr(zigbee_relay_cluster *cluster,
                                 uint16_t attribute_id);

void relay_cluster_store_attrs_to_nv(zigbee_relay_cluster *cluster);
void relay_cluster_load_attrs_from_nv(zigbee_relay_cluster *cluster);
void relay_cluster_handle_startup_mode(zigbee_relay_cluster *cluster);

void sync_indicator_led(zigbee_relay_cluster *cluster);

#ifdef HAL_HAS_COUNTDOWN_TIMER
void relay_cluster_task(void);

#endif

zigbee_relay_cluster *relay_cluster_by_endpoint[10];

void relay_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                  uint16_t attribute_id) {
    relay_cluster_on_write_attr(relay_cluster_by_endpoint[endpoint],
                                attribute_id);
}

void update_relay_clusters() {
    for (int i = 0; i < 10; i++) {
        if (relay_cluster_by_endpoint[i] != NULL) {
            sync_indicator_led(relay_cluster_by_endpoint[i]);
        }
    }
}

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint) {
    relay_cluster_by_endpoint[endpoint->endpoint] = cluster;
    cluster->endpoint = endpoint->endpoint;
    relay_cluster_load_attrs_from_nv(cluster);

    cluster->relay->callback_param = cluster;
    cluster->relay->on_change      = (relay_callback_t)relay_cluster_on_relay_change;

#ifdef HAL_HAS_COUNTDOWN_TIMER
    cluster->off_timer_active = 0;
#endif

    relay_cluster_handle_startup_mode(cluster);
    sync_indicator_led(cluster);

    SETUP_ATTR(0, ZCL_ATTR_ONOFF, ZCL_DATA_TYPE_BOOLEAN, ATTR_READONLY,
               cluster->relay->on);
    SETUP_ATTR(1, ZCL_ATTR_START_UP_ONOFF, ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE,
               cluster->startup_mode);
    if (cluster->indicator_led != NULL) {
        SETUP_ATTR(2, ZCL_ATTR_ONOFF_INDICATOR_MODE, ZCL_DATA_TYPE_ENUM8,
                   ATTR_WRITABLE, cluster->indicator_led_mode);
        SETUP_ATTR(3, ZCL_ATTR_ONOFF_INDICATOR_STATE, ZCL_DATA_TYPE_BOOLEAN,
                   ATTR_WRITABLE, cluster->indicator_state);
    }

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_ON_OFF;
    endpoint->clusters[endpoint->cluster_count].attribute_count =
        cluster->indicator_led != NULL ? 4 : 2;
    endpoint->clusters[endpoint->cluster_count].attributes   = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server    = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback =
        relay_cluster_callback_trampoline;
    endpoint->cluster_count++;

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_LEVEL_CONTROL;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
    endpoint->clusters[endpoint->cluster_count].attributes      = NULL;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    =
        relay_cluster_level_callback_trampoline;
    endpoint->cluster_count++;
}

hal_zigbee_cmd_result_t relay_cluster_callback_trampoline(uint8_t endpoint,
                                                          uint16_t cluster_id,
                                                          uint8_t command_id,
                                                          void *cmd_payload,
                                                          uint16_t cmd_payload_len) {
    return relay_cluster_callback(relay_cluster_by_endpoint[endpoint], command_id,
                                  cmd_payload, cmd_payload_len);
}

hal_zigbee_cmd_result_t relay_cluster_callback(zigbee_relay_cluster *cluster,
                                               uint8_t command_id,
                                               void *cmd_payload,
                                               uint16_t cmd_payload_len) {
    switch (command_id) {
    case ZCL_CMD_ONOFF_ON:
    case ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE:
#ifdef HAL_HAS_COUNTDOWN_TIMER
        cluster->off_timer_active = 0;
#endif
        relay_cluster_on(cluster);
        break;

    case ZCL_CMD_ONOFF_OFF:
    case ZCL_CMD_OFF_WITH_EFFECT:
#ifdef HAL_HAS_COUNTDOWN_TIMER
        cluster->off_timer_active = 0;
#endif
        relay_cluster_off(cluster);
        break;

    case ZCL_CMD_ONOFF_TOGGLE:
#ifdef HAL_HAS_COUNTDOWN_TIMER
        cluster->off_timer_active = 0;
#endif
        relay_cluster_toggle(cluster);
        break;

#ifdef HAL_HAS_COUNTDOWN_TIMER
    case ZCL_CMD_ON_WITH_TIMED_OFF: {
        // Payload layout (little-endian):
        //   [0] onOffCtrl, [1..2] onTime, [3..4] offWaitTime
        // onTime and offWaitTime are in units of 100 ms.
        if (cmd_payload == NULL || cmd_payload_len < 5) {
            return HAL_ZIGBEE_MALFORMED_COMMAND;
        }
        const uint8_t *p             = (const uint8_t *)cmd_payload;
        uint16_t       on_time       = (uint16_t)p[1] | ((uint16_t)p[2] << 8);
        uint16_t       off_wait_time = (uint16_t)p[3] | ((uint16_t)p[4] << 8);
        cluster->off_timer_active = 0;

        // ZCL case (0,0): turn OFF now.
        if (on_time == 0 && off_wait_time == 0) {
            relay_cluster_off(cluster);
            return HAL_ZIGBEE_CMD_PROCESSED;
        }
        if (on_time > 0) {
            // offWaitTime takes precedence when set, otherwise onTime.
            uint32_t delay_ms = off_wait_time > 0
                ? (uint32_t)off_wait_time * 100
                : (uint32_t)on_time * 100;
            if (delay_ms < 100) {
                delay_ms = 100;  // 0.1 s = 1 spec unit
            }
            // Drive the relay first: the on_change callback it fires
            // cancels any pending timer, and we arm the new one after it.
            relay_cluster_on(cluster);
            // The deadline lives in the 32-bit hal_millis() time domain.
            // relay_cluster_task() compares it with a signed difference,
            // so the wrap of the counter is handled.
            cluster->off_timer_deadline_ms = hal_millis() + delay_ms;
            cluster->off_timer_active      = 1;
            return HAL_ZIGBEE_CMD_PROCESSED;
        }
        // ZCL case onTime=0, offWaitTime>0: stay OFF for offWaitTime.
        // The "wait, then turn ON" half is not implemented.
        relay_cluster_off(cluster);
        return HAL_ZIGBEE_CMD_PROCESSED;
    }
#endif

    default:
        printf("Unknown OnOff command: %d\r\n", command_id);
        return HAL_ZIGBEE_CMD_SKIPPED;
    }
    return HAL_ZIGBEE_CMD_PROCESSED;
}

hal_zigbee_cmd_result_t relay_cluster_level_callback_trampoline(uint8_t endpoint,
                                                                uint16_t cluster_id,
                                                                uint8_t command_id,
                                                                void *cmd_payload,
                                                                uint16_t cmd_payload_len) {
    return relay_cluster_level_callback(relay_cluster_by_endpoint[endpoint], command_id,
                                        cmd_payload, cmd_payload_len);
}

hal_zigbee_cmd_result_t relay_cluster_level_callback(zigbee_relay_cluster *cluster,
                                                     uint8_t command_id,
                                                     void *cmd_payload,
                                                     uint16_t cmd_payload_len) {
    switch (command_id) {
    case ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF:
        if (cmd_payload == NULL || cmd_payload_len < 1) {
            return HAL_ZIGBEE_MALFORMED_COMMAND;
        }
        uint8_t level = *(uint8_t *)cmd_payload;
#ifdef HAL_HAS_COUNTDOWN_TIMER
        cluster->off_timer_active = 0;
#endif
        if (level == 0) {
            relay_cluster_off(cluster);
        } else {
            relay_cluster_on(cluster);
        }
        break;

    default:
        printf("Unknown LevelCtrl command: %d\r\n", command_id);
        return HAL_ZIGBEE_CMD_SKIPPED;
    }
    return HAL_ZIGBEE_CMD_PROCESSED;
}

void sync_indicator_led(zigbee_relay_cluster *cluster) {
    if (cluster->indicator_led == NULL) {
        return;
    }

    if (cluster->indicator_led_mode != ZCL_ONOFF_INDICATOR_MODE_MANUAL) {
        if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_SAME) {
            cluster->indicator_state = cluster->relay->on;
        } else {
            cluster->indicator_state = !cluster->relay->on;
        }
    }

    cluster->indicator_state ? led_on(cluster->indicator_led)
                           : led_off(cluster->indicator_led);

    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_ON_OFF,
                                        ZCL_ATTR_ONOFF_INDICATOR_STATE);
}

void relay_cluster_on(zigbee_relay_cluster *cluster) {
    relay_on(cluster->relay);
    sync_indicator_led(cluster);
}

void relay_cluster_off(zigbee_relay_cluster *cluster) {
    relay_off(cluster->relay);
    sync_indicator_led(cluster);
}

void relay_cluster_toggle(zigbee_relay_cluster *cluster) {
    relay_toggle(cluster->relay);
    sync_indicator_led(cluster);
}

void relay_cluster_on_relay_change(zigbee_relay_cluster *cluster,
                                   uint8_t state) {
#ifdef HAL_HAS_COUNTDOWN_TIMER
    // A local action (button) cancels a pending auto-off.
    cluster->off_timer_active = 0;
#endif
    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_ON_OFF,
                                        ZCL_ATTR_ONOFF);
    if (cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TOGGLE ||
        cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TO_PREVIOUS) {
        relay_cluster_store_attrs_to_nv(cluster);
    }
}

void relay_cluster_on_write_attr(zigbee_relay_cluster *cluster,
                                 uint16_t attribute_id) {
    if (attribute_id == ZCL_ATTR_ONOFF_INDICATOR_STATE) {
        sync_indicator_led(cluster);
    }
    if (cluster->indicator_led_mode != ZCL_ONOFF_INDICATOR_MODE_MANUAL) {
        sync_indicator_led(cluster);
    }

    relay_cluster_store_attrs_to_nv(cluster);
}

typedef struct {
    uint8_t on_off;
    uint8_t startup_mode;
    uint8_t indicator_led_mode;
    uint8_t indicator_led_on;
} zigbee_relay_cluster_config;

static zigbee_relay_cluster_config nv_config_buffer;

void relay_cluster_store_attrs_to_nv(zigbee_relay_cluster *cluster) {
    nv_config_buffer.on_off             = cluster->relay->on;
    nv_config_buffer.startup_mode       = cluster->startup_mode;
    nv_config_buffer.indicator_led_mode = cluster->indicator_led_mode;
    if (cluster->indicator_led != NULL) {
        nv_config_buffer.indicator_led_on = cluster->indicator_state;
    }

    hal_nvm_write(NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx),
                  sizeof(zigbee_relay_cluster_config),
                  (uint8_t *)&nv_config_buffer);
}

void relay_cluster_load_attrs_from_nv(zigbee_relay_cluster *cluster) {
    hal_nvm_status_t st = hal_nvm_read(
        NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx),
        sizeof(zigbee_relay_cluster_config), (uint8_t *)&nv_config_buffer);

    if (st != HAL_NVM_SUCCESS)
        return;

    cluster->startup_mode       = nv_config_buffer.startup_mode;
    cluster->indicator_led_mode = nv_config_buffer.indicator_led_mode;
    cluster->indicator_state    = nv_config_buffer.indicator_led_on;
}

void relay_cluster_handle_startup_mode(zigbee_relay_cluster *cluster) {
    hal_nvm_status_t st = hal_nvm_read(
        NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx),
        sizeof(zigbee_relay_cluster_config), (uint8_t *)&nv_config_buffer);

    if (st != HAL_NVM_SUCCESS)
        return;

    uint8_t prev_on = nv_config_buffer.on_off;

    switch (cluster->startup_mode) {
    case ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF:
        relay_cluster_off(cluster);
        break;

    case ZCL_START_UP_ONOFF_SET_ONOFF_TO_ON:
        relay_cluster_on(cluster);
        break;

    case ZCL_START_UP_ONOFF_SET_ONOFF_TOGGLE:
        if (prev_on) {
            relay_cluster_off(cluster);
        } else {
            relay_cluster_on(cluster);
        }
        break;

    case ZCL_START_UP_ONOFF_SET_ONOFF_TO_PREVIOUS:
        if (prev_on) {
            relay_cluster_on(cluster);
        } else {
            relay_cluster_off(cluster);
        }
        break;
    }

    // Restore indicator LED state
    sync_indicator_led(cluster);
}

#ifdef HAL_HAS_COUNTDOWN_TIMER
// Fire any expired auto-off timer. Called from app_task() every loop.
//
// The deadline is a 32-bit millisecond value and the comparison uses the
// signed difference of the two values, so it stays correct across the
// ~49.7 day wrap of hal_millis().
void relay_cluster_task(void) {
    const uint32_t now_ms = hal_millis();

    for (int ep = 0; ep < 10; ep++) {
        zigbee_relay_cluster *cluster = relay_cluster_by_endpoint[ep];
        if (cluster == NULL || cluster->off_timer_active == 0) {
            continue;
        }
        if ((int32_t)(now_ms - cluster->off_timer_deadline_ms) >= 0) {
            cluster->off_timer_active = 0;
            relay_cluster_off(cluster);
        }
    }
}

#endif
