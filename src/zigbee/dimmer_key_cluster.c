#include "dimmer_key_cluster.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/zigbee.h"
#include "zigbee_commands.h"

// ============================================================================
// Constants
// ============================================================================

#define MULTISTATE_RELEASED      0
#define MULTISTATE_UP_PRESS      1
#define MULTISTATE_DOWN_PRESS    2
#define MULTISTATE_UP_LONG       3
#define MULTISTATE_DOWN_LONG     4

static const uint8_t  multistate_out_of_service = 0;
static const uint8_t  multistate_flags          = 0;
static const uint16_t multistate_num_of_states  = 5;

static zigbee_dimmer_key_cluster *      dimmer_key_cluster_by_endpoint[10];
static zigbee_dimmer_key_cluster_config nv_config_buffer;

// ============================================================================
// NVM Persistence
// ============================================================================

static void dimmer_key_cluster_store_attrs_to_nv(zigbee_dimmer_key_cluster *cluster) {
    nv_config_buffer.button_long_press_duration =
        cluster->up_button->long_press_duration_ms;
    nv_config_buffer.level_move_rate = cluster->level_move_rate;
    hal_nvm_write(NV_ITEM_DIMMER_KEY_CONFIG(cluster->dimmer_key_idx),
                  sizeof(zigbee_dimmer_key_cluster_config),
                  (uint8_t *)&nv_config_buffer);
}

static void dimmer_key_cluster_load_attrs_from_nv(zigbee_dimmer_key_cluster *cluster) {
    hal_nvm_status_t st = hal_nvm_read(
        NV_ITEM_DIMMER_KEY_CONFIG(cluster->dimmer_key_idx),
        sizeof(zigbee_dimmer_key_cluster_config),
        (uint8_t *)&nv_config_buffer);

    if (st != HAL_NVM_SUCCESS) {
        printf("No dimmer key config in NV, using defaults\r\n");
        return;
    }

    cluster->up_button->long_press_duration_ms   = nv_config_buffer.button_long_press_duration;
    cluster->down_button->long_press_duration_ms = nv_config_buffer.button_long_press_duration;
    cluster->level_move_rate = nv_config_buffer.level_move_rate;
}

// ============================================================================
// Attribute Write Handler
// ============================================================================

static void dimmer_key_cluster_on_write_attr(zigbee_dimmer_key_cluster *cluster,
                                             uint16_t attribute_id) {
    if (attribute_id == ZCL_ATTR_DIMMER_KEY_CONFIG_LONG_PRESS_DUR) {
        // Long press duration is shared between up and down buttons
        cluster->down_button->long_press_duration_ms =
            cluster->up_button->long_press_duration_ms;
    }
    dimmer_key_cluster_store_attrs_to_nv(cluster);
}

void dimmer_key_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                       uint16_t attribute_id) {
    dimmer_key_cluster_on_write_attr(dimmer_key_cluster_by_endpoint[endpoint],
                                     attribute_id);
}

// ============================================================================
// Zigbee Command Helpers
// ============================================================================

static void send_onoff(zigbee_dimmer_key_cluster *cluster, uint8_t cmd_id) {
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        return;
    }
    hal_zigbee_cmd c = build_onoff_cmd(cluster->endpoint, cmd_id);
    hal_zigbee_send_cmd_to_bindings(&c);
}

static void send_level_move(zigbee_dimmer_key_cluster *cluster, uint8_t direction) {
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        return;
    }
    hal_zigbee_cmd c = build_level_move_onoff_cmd(cluster->endpoint, direction,
                                                  cluster->level_move_rate);
    hal_zigbee_send_cmd_to_bindings(&c);
}

static void send_level_stop(zigbee_dimmer_key_cluster *cluster) {
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        return;
    }
    hal_zigbee_cmd c = build_level_stop_onoff_cmd(cluster->endpoint);
    hal_zigbee_send_cmd_to_bindings(&c);
}

static void update_present_value(zigbee_dimmer_key_cluster *cluster,
                                 uint16_t value) {
    cluster->present_value = value;
    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
}

// ============================================================================
// Button Event Handlers
// ============================================================================

static void dimmer_key_cluster_on_up_press(zigbee_dimmer_key_cluster *cluster) {
    update_present_value(cluster, MULTISTATE_UP_PRESS);
}

static void dimmer_key_cluster_on_up_long_press(zigbee_dimmer_key_cluster *cluster) {
    update_present_value(cluster, MULTISTATE_UP_LONG);
    send_level_move(cluster, ZCL_LEVEL_MOVE_UP);
}

static void dimmer_key_cluster_on_up_release(zigbee_dimmer_key_cluster *cluster) {
    if (cluster->up_button->long_pressed) {
        send_level_stop(cluster);
    } else {
        send_onoff(cluster, ZCL_CMD_ONOFF_ON);
    }
    update_present_value(cluster, MULTISTATE_RELEASED);
}

static void dimmer_key_cluster_on_down_press(zigbee_dimmer_key_cluster *cluster) {
    update_present_value(cluster, MULTISTATE_DOWN_PRESS);
}

static void dimmer_key_cluster_on_down_long_press(zigbee_dimmer_key_cluster *cluster) {
    update_present_value(cluster, MULTISTATE_DOWN_LONG);
    send_level_move(cluster, ZCL_LEVEL_MOVE_DOWN);
}

static void dimmer_key_cluster_on_down_release(zigbee_dimmer_key_cluster *cluster) {
    if (cluster->down_button->long_pressed) {
        send_level_stop(cluster);
    } else {
        send_onoff(cluster, ZCL_CMD_ONOFF_OFF);
    }
    update_present_value(cluster, MULTISTATE_RELEASED);
}

// ============================================================================
// Initialization
// ============================================================================

void dimmer_key_cluster_add_to_endpoint(zigbee_dimmer_key_cluster *cluster,
                                        hal_zigbee_endpoint *endpoint) {
    dimmer_key_cluster_by_endpoint[endpoint->endpoint] = cluster;
    cluster->endpoint        = endpoint->endpoint;
    cluster->level_move_rate = 50;
    cluster->present_value   = MULTISTATE_RELEASED;

    dimmer_key_cluster_load_attrs_from_nv(cluster);

    cluster->up_button->on_press =
        (ev_button_callback_t)dimmer_key_cluster_on_up_press;
    cluster->up_button->on_release =
        (ev_button_callback_t)dimmer_key_cluster_on_up_release;
    cluster->up_button->on_long_press =
        (ev_button_callback_t)dimmer_key_cluster_on_up_long_press;
    cluster->up_button->callback_param = cluster;

    cluster->down_button->on_press =
        (ev_button_callback_t)dimmer_key_cluster_on_down_press;
    cluster->down_button->on_release =
        (ev_button_callback_t)dimmer_key_cluster_on_down_release;
    cluster->down_button->on_long_press =
        (ev_button_callback_t)dimmer_key_cluster_on_down_long_press;
    cluster->down_button->callback_param = cluster;

    // Configuration attributes (manufacturer-specific server cluster)
    SETUP_ATTR_FOR_TABLE(cluster->config_attr_infos, 0,
                         ZCL_ATTR_DIMMER_KEY_CONFIG_LONG_PRESS_DUR,
                         ZCL_DATA_TYPE_UINT16, ATTR_WRITABLE,
                         cluster->up_button->long_press_duration_ms);
    SETUP_ATTR_FOR_TABLE(cluster->config_attr_infos, 1,
                         ZCL_ATTR_DIMMER_KEY_CONFIG_LEVEL_MOVE_RATE,
                         ZCL_DATA_TYPE_UINT8, ATTR_WRITABLE,
                         cluster->level_move_rate);

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_DIMMER_KEY_CONFIG;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 2;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->config_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->cluster_count++;

    // OnOff client — for binding to lights
    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_ON_OFF;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
    endpoint->clusters[endpoint->cluster_count].attributes      = NULL;
    endpoint->clusters[endpoint->cluster_count].is_server       = 0;
    endpoint->cluster_count++;

    // LevelControl client — for binding to dimmers
    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_LEVEL_CONTROL;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
    endpoint->clusters[endpoint->cluster_count].attributes      = NULL;
    endpoint->clusters[endpoint->cluster_count].is_server       = 0;
    endpoint->cluster_count++;

    // MultistateInput — press action reporting
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 0,
                         ZCL_ATTR_MULTISTATE_INPUT_NUMBER_OF_STATES,
                         ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                         multistate_num_of_states);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 1,
                         ZCL_ATTR_MULTISTATE_INPUT_OUT_OF_SERVICE,
                         ZCL_DATA_TYPE_BOOLEAN, ATTR_READONLY,
                         multistate_out_of_service);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 2,
                         ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
                         ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                         cluster->present_value);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 3,
                         ZCL_ATTR_MULTISTATE_INPUT_STATUS_FLAGS,
                         ZCL_DATA_TYPE_BITMAP8, ATTR_READONLY,
                         multistate_flags);

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 4;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->multistate_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->cluster_count++;
}
