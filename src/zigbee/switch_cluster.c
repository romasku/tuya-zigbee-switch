#include "switch_cluster.h"
#include "base_components/relay.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"

#include "hal/printf_selector.h"
#include "hal/system.h"
#include "hal/tasks.h"
#include "relay_cluster.h"
#include "zigbee_commands.h"

const uint8_t multistate_out_of_service = 0;
const uint8_t multistate_flags          = 0;

// Backward-compat transient states (modes TOGGLE / MOMENTARY legacy)
#define MULTISTATE_NOT_PRESSED     0
#define MULTISTATE_PRESS           1
#define MULTISTATE_LONG_PRESS      2
#define MULTISTATE_POSITION_ON     3
#define MULTISTATE_POSITION_OFF    4

// Generic v2 event formula — works for any n (n=1,2,3,...):
//   n=1: press=5, hold=2 (backward compat), release=6
//   n=2: press=7, hold=8,  release=9
//   n=3: press=10, hold=11, release=12
//   n≥2: press=3n+1, hold=3n+2, release=3n+3
#define MULTISTATE_N_PRESS(n)      ((n) == 1u ? 5u : (3u * (n) + 1u))
#define MULTISTATE_N_HOLD(n)       ((n) == 1u ? MULTISTATE_LONG_PRESS : (3u * (n) + 2u))
#define MULTISTATE_N_RELEASE(n)    ((n) == 1u ? 6u : (3u * (n) + 3u))

// Default timer durations (used when cluster fields are 0)
#define DEFAULT_CONFIRM_RELEASE_MS    200u
#define DEFAULT_MAX_PRESS_COUNT       2u

extern zigbee_relay_cluster relay_clusters[];
extern uint8_t relay_clusters_cnt;
extern zigbee_switch_cluster switch_clusters[];
extern uint8_t switch_clusters_cnt;

void switch_cluster_on_button_press(zigbee_switch_cluster *cluster);
void switch_cluster_on_button_release(zigbee_switch_cluster *cluster);
void switch_cluster_timer_hold_cb(zigbee_switch_cluster *cluster);
void switch_cluster_timer_confirm_cb(zigbee_switch_cluster *cluster);
void switch_cluster_on_button_long_press(zigbee_switch_cluster *cluster);
static bool switch_cluster_has_valid_relay(
    const zigbee_switch_cluster *cluster);

zigbee_switch_cluster *switch_cluster_by_endpoint[10];

static void sync_switch_indicator_led(zigbee_switch_cluster *cluster) {
    if (cluster->indicator_led == NULL) {
        return;
    }

    if (cluster->relay_mode != ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED &&
        switch_cluster_has_valid_relay(cluster)) {
        return;
    }

    led_off(cluster->indicator_led);
}

void update_switch_clusters() {
    for (int i = 0; i < switch_clusters_cnt; i++) {
        sync_switch_indicator_led(&switch_clusters[i]);
    }
}

static bool switch_cluster_has_valid_relay(const zigbee_switch_cluster *cluster) {
    return cluster->relay_index > 0 && cluster->relay_index <= relay_clusters_cnt;
}

static void switch_cluster_flash_indicator(zigbee_switch_cluster *cluster) {
    if (cluster->indicator_led == NULL) {
        return;
    }
    // Skip flash when relay is attached — the relay toggle itself changes the
    // indicator, and the blink would race with sync_indicator_led.
    if (cluster->relay_mode != ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED &&
        switch_cluster_has_valid_relay(cluster)) {
        return;
    }
    // Only flash when LED is idle (not in "not connected" forever-blink)
    if (cluster->indicator_led->blink_times_left == 0) {
        led_blink(cluster->indicator_led, 50, 50, 1);
    }
}

void switch_cluster_store_attrs_to_nv(zigbee_switch_cluster *cluster);
void switch_cluster_load_attrs_from_nv(zigbee_switch_cluster *cluster);
void switch_cluster_on_write_attr(zigbee_switch_cluster *cluster,
                                  uint16_t attribute_id);

void switch_cluster_report_action(zigbee_switch_cluster *cluster);

void switch_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                   uint16_t attribute_id) {
    switch_cluster_on_write_attr(switch_cluster_by_endpoint[endpoint],
                                 attribute_id);
}

void switch_cluster_add_to_endpoint(zigbee_switch_cluster *cluster,
                                    hal_zigbee_endpoint *endpoint) {
    switch_cluster_by_endpoint[endpoint->endpoint] = cluster;
    cluster->endpoint = endpoint->endpoint;

    // Apply defaults for v2 fields when not set
    if (cluster->confirm_release_ms == 0)
        cluster->confirm_release_ms = DEFAULT_CONFIRM_RELEASE_MS;
    if (cluster->max_press_count == 0)
        cluster->max_press_count = DEFAULT_MAX_PRESS_COUNT;
    cluster->multistate_num_of_states = (uint16_t)(3u * cluster->max_press_count + 4u);

    switch_cluster_load_attrs_from_nv(cluster);

    // Wire only on_press and on_release; timer_hold replaces on_long_press
    cluster->button->on_press       = (ev_button_callback_t)switch_cluster_on_button_press;
    cluster->button->on_release     = (ev_button_callback_t)switch_cluster_on_button_release;
    cluster->button->on_long_press  = NULL;
    cluster->button->callback_param = cluster;

    // Initialise the two timers
    cluster->timer_hold.handler = (task_handler_t)switch_cluster_timer_hold_cb;
    cluster->timer_hold.arg     = cluster;
    hal_tasks_init(&cluster->timer_hold);

    cluster->timer_confirm.handler = (task_handler_t)switch_cluster_timer_confirm_cb;
    cluster->timer_confirm.arg     = cluster;
    hal_tasks_init(&cluster->timer_confirm);

    SETUP_ATTR(0, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE, ZCL_DATA_TYPE_ENUM8,
               ATTR_READONLY, cluster->mode);
    SETUP_ATTR(1, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
               ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE, cluster->action);
    SETUP_ATTR(2, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE, ZCL_DATA_TYPE_ENUM8,
               ATTR_WRITABLE, cluster->mode);
    SETUP_ATTR(3, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
               ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE, cluster->relay_mode);
    SETUP_ATTR(4, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
               ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE, cluster->relay_index);
    SETUP_ATTR(5, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
               ZCL_DATA_TYPE_UINT16, ATTR_WRITABLE,
               cluster->button->long_press_duration_ms);
    SETUP_ATTR(6, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
               ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE, cluster->level_move_rate);
    SETUP_ATTR(7, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
               ZCL_DATA_TYPE_ENUM8, ATTR_WRITABLE, cluster->binded_mode);
    // v2 attributes
    SETUP_ATTR(8, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_CONFIRM_RELEASE_DUR,
               ZCL_DATA_TYPE_UINT16, ATTR_WRITABLE, cluster->confirm_release_ms);
    SETUP_ATTR(9, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MAX_PRESS_COUNT,
               ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE, cluster->max_press_count);

    // Configuration cluster
    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 10;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->cluster_count++;

    // Output ON OFF to bind to other devices
    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_ON_OFF;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
    endpoint->clusters[endpoint->cluster_count].attributes      = NULL;
    endpoint->clusters[endpoint->cluster_count].is_server       = 0;
    endpoint->cluster_count++;

    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 0,
                         ZCL_ATTR_MULTISTATE_INPUT_NUMBER_OF_STATES,
                         ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                         cluster->multistate_num_of_states);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 1,
                         ZCL_ATTR_MULTISTATE_INPUT_OUT_OF_SERVICE,
                         ZCL_DATA_TYPE_BOOLEAN, ATTR_READONLY,
                         multistate_out_of_service);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 2,
                         ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
                         ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                         cluster->multistate_state);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 3,
                         ZCL_ATTR_MULTISTATE_INPUT_STATUS_FLAGS,
                         ZCL_DATA_TYPE_BITMAP8, ATTR_READONLY, multistate_flags);

    // Output
    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 4;
    endpoint->clusters[endpoint->cluster_count].attributes      =
        cluster->multistate_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server = 1;
    endpoint->cluster_count++;

    // Output Level for other devices
    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_LEVEL_CONTROL;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
    endpoint->clusters[endpoint->cluster_count].attributes      = NULL;
    endpoint->clusters[endpoint->cluster_count].is_server       = 0;
    endpoint->cluster_count++;
}

// Perform the relay action for ON position (position 1 in ZCL docs)
void switch_cluster_relay_action_on(zigbee_switch_cluster *cluster) {
    if (!switch_cluster_has_valid_relay(cluster))
        return;

    zigbee_relay_cluster *relay_cluster =
        &relay_clusters[cluster->relay_index - 1];

    switch (cluster->action) {
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
        relay_cluster_on(relay_cluster);
        break;
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
        relay_cluster_off(relay_cluster);
        break;
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
        relay_cluster_toggle(relay_cluster);
        break;
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC:
        relay_cluster_toggle(relay_cluster);
        break;
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE:
        relay_cluster_toggle(relay_cluster);
        break;
    }
}

// Perform the relay action for OFF position (position 2 in ZCL docs)
void switch_cluster_relay_action_off(zigbee_switch_cluster *cluster) {
    if (!switch_cluster_has_valid_relay(cluster))
        return;

    zigbee_relay_cluster *relay_cluster =
        &relay_clusters[cluster->relay_index - 1];

    switch (cluster->action) {
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
        relay_cluster_off(relay_cluster);
        break;
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
        relay_cluster_on(relay_cluster);
        break;
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
        relay_cluster_toggle(relay_cluster);
        break;
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC:
        relay_cluster_toggle(relay_cluster);
        break;
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE:
        relay_cluster_toggle(relay_cluster);
        break;
    }
}

// Send OnOff command to binded device based on ON position (position 1 in
// ZCL docs)
void switch_cluster_binding_action_on(zigbee_switch_cluster *cluster) {
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        return;
    }

    uint8_t cmd_id;

    switch (cluster->action) {
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
        cmd_id = ZCL_CMD_ONOFF_ON;
        break;

    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
        cmd_id = ZCL_CMD_ONOFF_OFF;
        break;

    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
        cmd_id = ZCL_CMD_ONOFF_TOGGLE;
        break;

    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC:
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE:
        if (!switch_cluster_has_valid_relay(cluster)) {
            cmd_id = ZCL_CMD_ONOFF_TOGGLE;
        } else {
            zigbee_relay_cluster *relay_cluster =
                &relay_clusters[cluster->relay_index - 1];
            if (cluster->action ==
                ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC)
                cmd_id = (relay_cluster->relay->on) ? ZCL_CMD_ONOFF_ON
                                                    : ZCL_CMD_ONOFF_OFF;
            else
                cmd_id = (relay_cluster->relay->on) ? ZCL_CMD_ONOFF_OFF
                                                    : ZCL_CMD_ONOFF_ON;
        }
        break;

    default:
        return;
    }

    hal_zigbee_cmd c = build_onoff_cmd(cluster->endpoint, cmd_id);
    hal_zigbee_send_cmd_to_bindings(&c);
    switch_cluster_flash_indicator(cluster);
}

// Send OnOff command to binded device based on OFF position (position 2 in
// ZCL docs)
void switch_cluster_binding_action_off(zigbee_switch_cluster *cluster) {
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        return;
    }

    uint8_t cmd_id;

    switch (cluster->action) {
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF:
        cmd_id = ZCL_CMD_ONOFF_OFF;
        break;

    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON:
        cmd_id = ZCL_CMD_ONOFF_ON;
        break;

    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE:
        cmd_id = ZCL_CMD_ONOFF_TOGGLE;
        break;

    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC:
    case ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE:
        if (!switch_cluster_has_valid_relay(cluster)) {
            cmd_id = ZCL_CMD_ONOFF_TOGGLE;
        } else {
            zigbee_relay_cluster *relay_cluster =
                &relay_clusters[cluster->relay_index - 1];
            if (cluster->action ==
                ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC)
                cmd_id = (relay_cluster->relay->on) ? ZCL_CMD_ONOFF_ON
                                                    : ZCL_CMD_ONOFF_OFF;
            else
                cmd_id = (relay_cluster->relay->on) ? ZCL_CMD_ONOFF_OFF
                                                    : ZCL_CMD_ONOFF_ON;
        }
        break;

    default:
        return;
    }

    hal_zigbee_cmd c = build_onoff_cmd(cluster->endpoint, cmd_id);
    hal_zigbee_send_cmd_to_bindings(&c);
    switch_cluster_flash_indicator(cluster);
}

void switch_cluster_level_stop(zigbee_switch_cluster *cluster) {
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        return;
    }

    hal_zigbee_cmd c = build_level_stop_onoff_cmd(cluster->endpoint);
    hal_zigbee_send_cmd_to_bindings(&c);
    switch_cluster_flash_indicator(cluster);
}

void switch_cluster_level_control(zigbee_switch_cluster *cluster) {
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        return;
    }

    hal_zigbee_cmd c = build_level_move_onoff_cmd(cluster->endpoint,
                                                  cluster->level_move_direction,
                                                  cluster->level_move_rate);
    hal_zigbee_send_cmd_to_bindings(&c);
    switch_cluster_flash_indicator(cluster);

    if (cluster->level_move_direction == ZCL_LEVEL_MOVE_DOWN) {
        cluster->level_move_direction = ZCL_LEVEL_MOVE_UP;
    } else {
        cluster->level_move_direction = ZCL_LEVEL_MOVE_DOWN;
    }
}

// Timer hold callback — fires after long_press_duration_ms while pressed
void switch_cluster_timer_hold_cb(zigbee_switch_cluster *cluster) {
    if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
        return;
    }
    hal_tasks_unschedule(&cluster->timer_confirm);
    cluster->in_hold = 1;

    // Backward-compat: legacy LONG relay/binding modes
    if (cluster->relay_mode == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG) {
        if (cluster->relay_index > 0) {
            relay_cluster_toggle(&relay_clusters[cluster->relay_index - 1]);
        }
    }
    if (cluster->binded_mode == ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG) {
        switch_cluster_binding_action_on(cluster);
    }
    switch_cluster_level_control(cluster);

    cluster->multistate_state = MULTISTATE_N_HOLD(cluster->n_press);
    printf("sw[%d] hold n=%d ms=%d\r\n", cluster->endpoint, cluster->n_press,
           cluster->multistate_state);
    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

// Timer confirm callback — fires after confirm_release_ms when not in hold
void switch_cluster_timer_confirm_cb(zigbee_switch_cluster *cluster) {
    if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
        return;
    }
    cluster->multistate_state = MULTISTATE_N_PRESS(cluster->n_press);
    printf("sw[%d] confirm n=%d ms=%d\r\n", cluster->endpoint, cluster->n_press,
           cluster->multistate_state);
    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
    cluster->n_press = 0;
    cluster->in_hold = 0;
    // multistate_state stays at the press value until next button activity
}

void switch_cluster_on_button_press(zigbee_switch_cluster *cluster) {
    switch_cluster_flash_indicator(cluster);

    if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
        // Toggle does not support modes (RISE, SHORT, LONG)
        if (cluster->relay_mode != ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED) {
            switch_cluster_relay_action_on(cluster);
        }
        switch_cluster_binding_action_on(cluster);
        cluster->multistate_state = MULTISTATE_POSITION_ON;
        hal_zigbee_notify_attribute_changed(
            cluster->endpoint, ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
            ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
        return;
    }

    // 2-timer machine: stop confirm timer, increment counter, arm hold timer
    hal_tasks_unschedule(&cluster->timer_confirm);
    if (cluster->n_press < cluster->max_press_count) {
        cluster->n_press++;
    }
    hal_tasks_schedule(&cluster->timer_hold, cluster->button->long_press_duration_ms);

    if (cluster->relay_mode == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE) {
        switch_cluster_relay_action_on(cluster);
    }
    if (cluster->binded_mode == ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE) {
        switch_cluster_binding_action_on(cluster);
    }

    cluster->multistate_state = MULTISTATE_PRESS;
    printf("sw[%d] press n=%d ms=%d\r\n", cluster->endpoint, cluster->n_press,
           cluster->multistate_state);
    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

void switch_cluster_on_button_release(zigbee_switch_cluster *cluster) {
    if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
        // Only flash on release for toggles,
        // for momentary flash on press only
        switch_cluster_flash_indicator(cluster);
    }

    if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
        // Toggle does not support modes (RISE, SHORT, LONG)
        if (cluster->relay_mode != ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED) {
            switch_cluster_relay_action_off(cluster);
        }
        switch_cluster_binding_action_off(cluster);
        cluster->multistate_state = MULTISTATE_POSITION_OFF;
        hal_zigbee_notify_attribute_changed(
            cluster->endpoint, ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
            ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
        return;
    }

    hal_tasks_unschedule(&cluster->timer_hold);

    if (cluster->in_hold) {
        // End of hold: emit release multistate briefly, then reset
        cluster->multistate_state = MULTISTATE_N_RELEASE(cluster->n_press);
        printf("sw[%d] release hold n=%d ms=%d\r\n", cluster->endpoint, cluster->n_press,
               cluster->multistate_state);
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                            ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
        switch_cluster_level_stop(cluster);
        cluster->n_press = 0;
        cluster->in_hold = 0;
    } else {
        // Short release: legacy SHORT mode fires immediately
        if (cluster->relay_mode == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT) {
            switch_cluster_relay_action_on(cluster);
        }
        if (cluster->binded_mode == ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT) {
            switch_cluster_binding_action_on(cluster);
        }
        // Arm confirm timer for new action_N_press
        hal_tasks_schedule(&cluster->timer_confirm, cluster->confirm_release_ms);
    }

    cluster->multistate_state = MULTISTATE_NOT_PRESSED;
    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

void synchronize_multistate_state(zigbee_switch_cluster *cluster) {
    if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE) {
        if (cluster->button->pressed) {
            cluster->multistate_state = MULTISTATE_POSITION_ON;
        } else {
            cluster->multistate_state = MULTISTATE_POSITION_OFF;
        }
    } else {
        if (cluster->in_hold) {
            cluster->multistate_state = MULTISTATE_LONG_PRESS;
        } else if (cluster->button->pressed) {
            cluster->multistate_state = MULTISTATE_PRESS;
        } else {
            cluster->multistate_state = MULTISTATE_NOT_PRESSED;
        }
    }
    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

void switch_cluster_on_write_attr(zigbee_switch_cluster *cluster,
                                  uint16_t attribute_id) {
    if (attribute_id == ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX) {
        if (relay_clusters_cnt == 0) {
            cluster->relay_index = 0;
        } else if (cluster->relay_index < 1 || cluster->relay_index > relay_clusters_cnt) {
            cluster->relay_index = 1;
        }
    }
    if (attribute_id == ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE) {
        synchronize_multistate_state(cluster);
        if (cluster->mode == ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY_NC) {
            cluster->button->pressed_when_high = 1;
        } else {
            cluster->button->pressed_when_high = 0;
        }
    }
    switch_cluster_store_attrs_to_nv(cluster);
}

zigbee_switch_cluster_config nv_config_buffer;

void switch_cluster_store_attrs_to_nv(zigbee_switch_cluster *cluster) {
    nv_config_buffer.action      = cluster->action;
    nv_config_buffer.mode        = cluster->mode;
    nv_config_buffer.relay_index = cluster->relay_index;
    nv_config_buffer.relay_mode  = cluster->relay_mode;
    nv_config_buffer.button_long_press_duration =
        cluster->button->long_press_duration_ms;
    nv_config_buffer.level_move_rate    = cluster->level_move_rate;
    nv_config_buffer.binded_mode        = cluster->binded_mode;
    nv_config_buffer.confirm_release_ms = cluster->confirm_release_ms;
    nv_config_buffer.max_press_count    = cluster->max_press_count;
    hal_nvm_write(NV_ITEM_SWITCH_CLUSTER_DATA(cluster->switch_idx),
                  sizeof(zigbee_switch_cluster_config),
                  (uint8_t *)&nv_config_buffer);
}

void switch_cluster_load_attrs_from_nv(zigbee_switch_cluster *cluster) {
    hal_nvm_status_t st = hal_nvm_read(
        NV_ITEM_SWITCH_CLUSTER_DATA(cluster->switch_idx),
        sizeof(zigbee_switch_cluster_config), (uint8_t *)&nv_config_buffer);

    if (st != HAL_NVM_SUCCESS) {
        printf("No switch config in NV, using defaults\r\n");
        return;
    }
    cluster->action      = nv_config_buffer.action;
    cluster->mode        = nv_config_buffer.mode;
    cluster->relay_index = nv_config_buffer.relay_index;
    cluster->relay_mode  = nv_config_buffer.relay_mode;
    cluster->button->long_press_duration_ms =
        nv_config_buffer.button_long_press_duration;
    cluster->level_move_rate    = nv_config_buffer.level_move_rate;
    cluster->binded_mode        = nv_config_buffer.binded_mode;
    cluster->confirm_release_ms = nv_config_buffer.confirm_release_ms;
    cluster->max_press_count    = nv_config_buffer.max_press_count;
    // Apply defaults for v2 fields when zero (upgraded device)
    if (cluster->confirm_release_ms == 0)
        cluster->confirm_release_ms = DEFAULT_CONFIRM_RELEASE_MS;
    if (cluster->max_press_count == 0)
        cluster->max_press_count = DEFAULT_MAX_PRESS_COUNT;
    cluster->multistate_num_of_states = (uint16_t)(3u * cluster->max_press_count + 4u);

    // Validate relay_index to prevent out-of-bounds access
    if (relay_clusters_cnt == 0) {
        cluster->relay_index = 0;
    } else if (cluster->relay_index < 1 || cluster->relay_index > relay_clusters_cnt) {
        printf("Invalid relay_index %d in NV, resetting to default\r\n",
               cluster->relay_index);
        cluster->relay_index = cluster->switch_idx + 1;
    }
}
