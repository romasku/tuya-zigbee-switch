#include "relay_cluster.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/timer.h"
#include "hal/printf_selector.h"
#include <stddef.h>

// ---------------------------------------------------------------------------
// onWithTimedOff payload layout.
//
// The Telink SDK (telink_tools/sdk/zigbee/zcl/general/zcl_onoff.c,
// case ZCL_CMD_ON_WITH_TIMED_OFF) parses the 5-byte payload into a
// local zcl_onoff_onWithTimeOffCmd_t on its stack. On tc32 the struct
// is PACKED (1-byte onOffCtrl, then 2-byte LE u16s), so the bytes
// in memory match the wire exactly:
//
//   offset 0: onOffCtrl     (1 byte, ignored)
//   offset 1: onTime LO     (1 byte)
//   offset 2: onTime HI     (1 byte)
//   offset 3: offWaitTime LO (1 byte)
//   offset 4: offWaitTime HI (1 byte)
//
// We read onTime / offWaitTime as LE u16s directly from the wire
// bytes. This is byte-order-safe across both tc32 and the x86_64
// host stub: the original Telink SDK on tc32 emits
// `(pData[2] << 8) | pData[1]` to compute onTime, so pData[1] is
// the LO byte. A struct cast would NOT work portably because the
// x86_64 default ABI gives the bitfield union 4 bytes (vs 1 on
// tc32), shifting every field by 3 — see the disassembly of the
// SDK's handler at zcl_onoff.c.
// ---------------------------------------------------------------------------

zigbee_relay_cluster *relay_cluster_by_endpoint[10];

// Forward decls (C99, no implicit function declarations) for the
// on-change / write-attr / NVM helpers that the ZCL dispatch and
// polling tick reference.
void sync_indicator_led(zigbee_relay_cluster *cluster);
void relay_cluster_on_relay_change(zigbee_relay_cluster *cluster,
                                   uint8_t state);
void relay_cluster_store_attrs_to_nv(zigbee_relay_cluster *cluster);

// ---------------------------------------------------------------------------
// ZCL command handler.
//
// The auto-OFF timer is two fields on the cluster struct
// (off_timer_active + off_timer_deadline_ms). The deadline is set
// BEFORE relay_cluster_on() fires the on_change callback — that
// callback calls relay_cluster_on_relay_change which would otherwise
// cancel a just-armed timer. With the deadline already set the cancel
// is a no-op.
// ---------------------------------------------------------------------------

hal_zigbee_cmd_result_t relay_cluster_callback(zigbee_relay_cluster *cluster,
                                               uint8_t command_id,
                                               void *cmd_payload,
                                               uint16_t cmd_payload_len) {
    switch (command_id) {
    case ZCL_CMD_ONOFF_ON:
    case ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE:
        cluster->off_timer_active = 0;
        relay_cluster_on(cluster);
        break;

    case ZCL_CMD_ONOFF_OFF:
    case ZCL_CMD_OFF_WITH_EFFECT:
        cluster->off_timer_active = 0;
        relay_cluster_off(cluster);
        break;

    case ZCL_CMD_ONOFF_TOGGLE:
        cluster->off_timer_active = 0;
        relay_cluster_toggle(cluster);
        break;

    case ZCL_CMD_ON_WITH_TIMED_OFF: {
#ifndef END_DEVICE
        // The auto-OFF timer is a polling tick driven by hal_millis_64(),
        // which is based on the 16MHz system clock. On end_device
        // builds the chip enters PM_SLEEP_MODE_SUSPEND between polls
        // and the system clock stops, so the timer cannot fire
        // reliably (see issue: "end_device sleep model is broken").
        // For end_device we accept the command but do not arm a
        // timer — the relay is still turned ON for the user, but
        // never auto-OFFs.
        if (cmd_payload == NULL || cmd_payload_len < 5) {
            return HAL_ZIGBEE_MALFORMED_COMMAND;
        }
        const uint8_t *p = (const uint8_t *)cmd_payload;
        // Telink SDK writes onTime / offWaitTime as little-endian u16
        // at offsets 1..2 and 3..4 of the 5-byte payload.
        uint16_t on_time      = (uint16_t)p[1] | ((uint16_t)p[2] << 8);
        uint16_t off_wait_time = (uint16_t)p[3] | ((uint16_t)p[4] << 8);
        cluster->off_timer_active = 0;

        // ZCL spec case (0,0): turn OFF now.
        if (on_time == 0 && off_wait_time == 0) {
            relay_cluster_off(cluster);
            return HAL_ZIGBEE_CMD_PROCESSED;
        }
        // ZCL spec case onTime>0: turn ON, schedule auto-OFF.
        //   offWaitTime>0 -> auto-OFF after offWaitTime.
        //   offWaitTime=0 -> auto-OFF after onTime.
        if (on_time > 0) {
            uint32_t delay_ms = off_wait_time > 0
                ? (uint32_t)off_wait_time * 100
                : (uint32_t)on_time * 100;
            if (delay_ms < 100) {
                delay_ms = 100;  // 0.1 s = 1 spec unit
            }
            // Drive the relay FIRST. The on_change callback fired by
            // relay_on() cancels any pending timer (a physical button
            // press or a fresh ZCL command is exactly what should
            // cancel a pending auto-OFF). We arm the timer AFTER that
            // callback, so the cancel can only ever clear a *previous*
            // timer, not the one we are installing right now.
            relay_cluster_on(cluster);
            cluster->off_timer_deadline_ms =
                hal_millis_64() + (uint64_t)delay_ms;
            cluster->off_timer_active = 1;
            return HAL_ZIGBEE_CMD_PROCESSED;
        }
        // ZCL spec case onTime=0 && offWaitTime>0: device must stay
        // OFF for offWaitTime. The romasku firmware does not
        // implement the "wait then ON" half — the command reduces
        // to a plain OFF (Z2M does not exercise this case).
        relay_cluster_off(cluster);
        return HAL_ZIGBEE_CMD_PROCESSED;
#else
        // end_device: hal_millis_64() does not survive PM_SLEEP_MODE_SUSPEND.
        // Turn the relay ON, but do not arm any timer — the auto-OFF
        // cannot fire reliably until the time source is fixed.
        (void)cmd_payload;
        (void)cmd_payload_len;
        relay_cluster_on(cluster);
        return HAL_ZIGBEE_CMD_PROCESSED;
#endif
    }

    default:
        printf("Unknown OnOff command: %d\r\n", command_id);
        return HAL_ZIGBEE_CMD_SKIPPED;
    }
    return HAL_ZIGBEE_CMD_PROCESSED;
}

hal_zigbee_cmd_result_t relay_cluster_callback_trampoline(uint8_t endpoint,
                                                          uint16_t cluster_id,
                                                          uint8_t command_id,
                                                          void *cmd_payload,
                                                          uint16_t cmd_payload_len) {
    return relay_cluster_callback(relay_cluster_by_endpoint[endpoint], command_id,
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
        cluster->off_timer_active = 0;
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

hal_zigbee_cmd_result_t relay_cluster_level_callback_trampoline(uint8_t endpoint,
                                                                uint16_t cluster_id,
                                                                uint8_t command_id,
                                                                void *cmd_payload,
                                                                uint16_t cmd_payload_len) {
    return relay_cluster_level_callback(relay_cluster_by_endpoint[endpoint], command_id,
                                        cmd_payload, cmd_payload_len);
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
    // Physical button / local action: cancel any pending auto-OFF so
    // the user gets a predictable, manually controlled state.
    cluster->off_timer_active = 0;

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

    sync_indicator_led(cluster);
}

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint) {
    relay_cluster_by_endpoint[endpoint->endpoint] = cluster;
    cluster->endpoint = endpoint->endpoint;
    relay_cluster_load_attrs_from_nv(cluster);

    cluster->relay->callback_param = cluster;
    cluster->relay->on_change      = (relay_callback_t)relay_cluster_on_relay_change;

    cluster->off_timer_active = 0;

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

void update_relay_clusters() {
    for (int i = 0; i < 10; i++) {
        if (relay_cluster_by_endpoint[i] != NULL) {
            sync_indicator_led(relay_cluster_by_endpoint[i]);
        }
    }
}

void relay_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                  uint16_t attribute_id) {
    relay_cluster_on_write_attr(relay_cluster_by_endpoint[endpoint],
                                attribute_id);
}

// ---------------------------------------------------------------------------
// Polling tick — the only place the auto-OFF timer fires.
//
// Called from app_task() every main loop iteration. One hal_millis_64()
// read per pass. No attribute reports are ever pushed on the wire —
// the firmware only ever sends traffic in response to commands or
// attribute writes.
// ---------------------------------------------------------------------------

void relay_cluster_task(void) {
#ifndef END_DEVICE
    // The polling tick uses hal_millis_64(), which is based on the
    // 16MHz system clock and does not survive PM_SLEEP_MODE_SUSPEND
    // on end_device builds. See the comment in the onWithTimedOff
    // case above for the full rationale.
    const uint64_t now_ms = hal_millis_64();

    for (uint8_t ep = 0; ep < 10; ep++) {
        zigbee_relay_cluster *cluster = relay_cluster_by_endpoint[ep];
        if (cluster == NULL) {
            continue;
        }
        if (cluster->off_timer_active == 0) {
            continue;
        }
        if (now_ms >= cluster->off_timer_deadline_ms) {
            cluster->off_timer_active = 0;
            relay_cluster_off(cluster);
        }
    }
#endif
}
