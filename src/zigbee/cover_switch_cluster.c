#include "cover_switch_cluster.h"
#include "basic_cluster.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/system.h"
#include "hal/tasks.h"
#include "hal/zigbee.h"
#include "cover_cluster.h"
#include "zigbee_commands.h"

// ============================================================================
// Configuration & Constants
// ============================================================================

#define MULTISTATE_RELEASED      0
#define MULTISTATE_OPEN          1
#define MULTISTATE_CLOSE         2
#define MULTISTATE_STOP          3
#define MULTISTATE_LONG_OPEN     4
#define MULTISTATE_LONG_CLOSE    5

extern zigbee_cover_cluster cover_clusters[];
extern uint8_t cover_clusters_cnt;

static zigbee_cover_switch_cluster *      cover_switch_cluster_by_endpoint[10];
static zigbee_cover_switch_cluster_config nv_config_buffer;

static const uint8_t  multistate_out_of_service = 0;
static const uint8_t  multistate_flags          = 0;
static const uint16_t multistate_num_of_states  = 6;

// ============================================================================
// Input Handling
// ============================================================================

uint8_t cover_switch_cluster_get_cmd(zigbee_cover_switch_cluster *cluster, uint8_t present_value,
                                     uint8_t mode, uint8_t moving) {
    if (present_value == MULTISTATE_STOP) {
        return(ZCL_CMD_WINDOW_COVERING_STOP);
    }

    if (cluster->switch_type == ZCL_COVER_SWITCH_TYPE_TOGGLE) {
        if (present_value == MULTISTATE_OPEN) {
            return(ZCL_CMD_WINDOW_COVERING_UP_OPEN);
        }else if (present_value == MULTISTATE_CLOSE) {
            return(ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE);
        }
    }else{
        uint8_t cmd = 0xFF;
        switch (mode) {
        case ZCL_COVER_SWITCH_MODE_IMMEDIATE:
            if (present_value == MULTISTATE_OPEN) {
                cmd = ZCL_CMD_WINDOW_COVERING_UP_OPEN;
            }else if (present_value == MULTISTATE_CLOSE) {
                cmd = ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE;
            }
            break;
        case ZCL_COVER_SWITCH_MODE_SHORT_PRESS:
            if (present_value == MULTISTATE_RELEASED) {
                if (cluster->present_value == MULTISTATE_OPEN) {
                    cmd = ZCL_CMD_WINDOW_COVERING_UP_OPEN;
                }else if (cluster->present_value == MULTISTATE_CLOSE) {
                    cmd = ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE;
                }
            }
            break;
        case ZCL_COVER_SWITCH_MODE_LONG_PRESS:
            if (present_value == MULTISTATE_LONG_OPEN) {
                cmd = ZCL_CMD_WINDOW_COVERING_UP_OPEN;
            }else if (present_value == MULTISTATE_LONG_CLOSE) {
                cmd = ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE;
            }
            break;
        case ZCL_COVER_SWITCH_MODE_HYBRID:
            if (present_value == MULTISTATE_LONG_OPEN) {
                return(ZCL_CMD_WINDOW_COVERING_UP_OPEN);
            }else if (present_value == MULTISTATE_LONG_CLOSE) {
                return(ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE);
            }else if (present_value == MULTISTATE_RELEASED) {
                if (cluster->present_value == MULTISTATE_OPEN) {
                    cmd = ZCL_CMD_WINDOW_COVERING_UP_OPEN;
                }else if (cluster->present_value == MULTISTATE_CLOSE) {
                    cmd = ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE;
                }else if (cluster->present_value == MULTISTATE_LONG_OPEN ||
                          cluster->present_value == MULTISTATE_LONG_CLOSE) {
                    return(ZCL_CMD_WINDOW_COVERING_STOP);
                }
            }
            break;
        }

        // Momentary switches send STOP command on repeated presses
        if (cmd != 0xFF) {
            if (cmd == ZCL_CMD_WINDOW_COVERING_UP_OPEN &&
                moving == ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING) {
                return(ZCL_CMD_WINDOW_COVERING_STOP);
            }else if (cmd == ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE &&
                      moving == ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING) {
                return(ZCL_CMD_WINDOW_COVERING_STOP);
            }

            return(cmd);
        }
    }

    return(0xFF);
}

void cover_switch_trigger_local_cmd(zigbee_cover_switch_cluster *cluster, uint8_t cmd) {
    zigbee_cover_cluster *output = &cover_clusters[cluster->cover_index - 1];

    switch (cmd) {
    case ZCL_CMD_WINDOW_COVERING_UP_OPEN:
        cover_open(output);
        break;
    case ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE:
        cover_close(output);
        break;
    case ZCL_CMD_WINDOW_COVERING_STOP:
        cover_stop(output);
        break;
    default:
        printf("Unknown cmd %d\r\n", cmd);
        break;
    }
}

void cover_switch_trigger_binding_cmd(zigbee_cover_switch_cluster *cluster, uint8_t cmd) {
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED) {
        return;
    }

    if (cmd == ZCL_CMD_WINDOW_COVERING_UP_OPEN) {
        cluster->binded_moving = ZCL_ATTR_WINDOW_COVERING_MOVING_OPENING;
    }else if (cmd == ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE) {
        cluster->binded_moving = ZCL_ATTR_WINDOW_COVERING_MOVING_CLOSING;
    }else{
        cluster->binded_moving = ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED;
    }

    hal_zigbee_cmd c = build_window_covering_cmd(cluster->endpoint, cmd);
    hal_zigbee_send_cmd_to_bindings(&c);
}

void cover_switch_cluster_update_present_value(zigbee_cover_switch_cluster *cluster,
                                               uint8_t present_value) {
    if (present_value == cluster->present_value) {
        return;
    }

    // When local & binded modes match: use local cover's actual state for both to keep them synchronized
    // When modes differ (or no local cover): track binded state separately via last-triggered command
    uint8_t local_cmd  = 0xFF;
    uint8_t binded_cmd = 0xFF;
    if (cluster->cover_index != 0 && cluster->cover_index <= cover_clusters_cnt) {
        zigbee_cover_cluster *output = &cover_clusters[cluster->cover_index - 1];
        local_cmd = cover_switch_cluster_get_cmd(cluster, present_value, cluster->local_mode,
                                                 output->moving);
        if (cluster->local_mode == cluster->binded_mode) {
            binded_cmd = local_cmd;
        }
    }

    if (binded_cmd == 0xFF) {
        binded_cmd = cover_switch_cluster_get_cmd(cluster, present_value, cluster->binded_mode,
                                                  cluster->binded_moving);
    }

    cluster->present_value = present_value;
    hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                        ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);

    if (local_cmd != 0xFF) {
        cover_switch_trigger_local_cmd(cluster, local_cmd);
    }

    if (binded_cmd != 0xFF) {
        cover_switch_trigger_binding_cmd(cluster, binded_cmd);
    }
}

void cover_switch_cluster_on_button_press(zigbee_cover_switch_cluster *cluster) {
    if (cluster->open_button->pressed && cluster->close_button->pressed) {
        cover_switch_cluster_update_present_value(cluster, MULTISTATE_STOP);
    }else if (cluster->open_button->pressed) {
        cover_switch_cluster_update_present_value(cluster,
                                                  cluster->reversal ? MULTISTATE_CLOSE :
                                                  MULTISTATE_OPEN);
    }else if (cluster->close_button->pressed) {
        cover_switch_cluster_update_present_value(cluster,
                                                  cluster->reversal ? MULTISTATE_OPEN :
                                                  MULTISTATE_CLOSE);
    }else{
        printf("Button press detected, but no button is pressed\r\n");
    }
}

void cover_switch_cluster_on_button_long_press(zigbee_cover_switch_cluster *cluster) {
    if (cluster->switch_type == ZCL_COVER_SWITCH_TYPE_TOGGLE) {
        // Toggle does not support long press
        return;
    }

    if (cluster->open_button->pressed && cluster->close_button->pressed) {
        // We don't care about long presses in stop state
        return;
    }

    if (cluster->open_button->long_pressed) {
        cover_switch_cluster_update_present_value(cluster,
                                                  cluster->reversal ? MULTISTATE_LONG_CLOSE :
                                                  MULTISTATE_LONG_OPEN);
    }else if (cluster->close_button->long_pressed) {
        cover_switch_cluster_update_present_value(cluster,
                                                  cluster->reversal ? MULTISTATE_LONG_OPEN :
                                                  MULTISTATE_LONG_CLOSE);
    }else{
        printf("Button long press detected, but no button is long pressed\r\n");
    }
}

void cover_switch_cluster_on_button_release(zigbee_cover_switch_cluster *cluster) {
    if (!cluster->open_button->pressed && !cluster->close_button->pressed) {
        uint8_t action = cluster->switch_type ==
                         ZCL_COVER_SWITCH_TYPE_TOGGLE ? MULTISTATE_STOP : MULTISTATE_RELEASED;
        cover_switch_cluster_update_present_value(cluster, action);
        return;
    }

    if (cluster->switch_type == ZCL_COVER_SWITCH_TYPE_MOMENTARY) {
        // Momentary switches must not revert back to OPEN or CLOSE when one of the buttons is released, otherwise it would
        // generate unwanted commands when the stop button is released.
        return;
    }

    // Regular toggle-type cover switches won't be able to close both contacts at the same time, but it's possible to get
    // to this state if a regular 2-gang switch is used as cover switch.
    if (cluster->open_button->pressed) {
        cover_switch_cluster_update_present_value(cluster,
                                                  cluster->reversal ? MULTISTATE_CLOSE :
                                                  MULTISTATE_OPEN);
    }else if (cluster->close_button->pressed) {
        cover_switch_cluster_update_present_value(cluster,
                                                  cluster->reversal ? MULTISTATE_OPEN :
                                                  MULTISTATE_CLOSE);
    }
}

// ============================================================================
// NVM Persistence
// ============================================================================

void cover_switch_cluster_store_attrs_to_nv(zigbee_cover_switch_cluster *cluster) {
    nv_config_buffer.cover_index = cluster->cover_index;
    nv_config_buffer.reversal    = cluster->reversal;
    nv_config_buffer.local_mode  = cluster->local_mode;
    nv_config_buffer.binded_mode = cluster->binded_mode;
    nv_config_buffer.switch_type = cluster->switch_type;

    hal_nvm_write(NV_ITEM_COVER_SWITCH_CONFIG(cluster->cover_switch_idx),
                  sizeof(zigbee_cover_switch_cluster_config),
                  (uint8_t *)&nv_config_buffer);
}

void cover_switch_cluster_load_attrs_from_nv(zigbee_cover_switch_cluster *cluster) {
    hal_nvm_status_t st = hal_nvm_read(
        NV_ITEM_COVER_SWITCH_CONFIG(cluster->cover_switch_idx),
        sizeof(zigbee_cover_switch_cluster_config),
        (uint8_t *)&nv_config_buffer);

    if (st != HAL_NVM_SUCCESS) {
        printf("No cover switch config in NV, using defaults\r\n");
        return;
    }

    cluster->cover_index = nv_config_buffer.cover_index;
    cluster->reversal    = nv_config_buffer.reversal;
    cluster->local_mode  = nv_config_buffer.local_mode;
    cluster->binded_mode = nv_config_buffer.binded_mode;
    cluster->switch_type = nv_config_buffer.switch_type;
}

// ============================================================================
// Attribute Handlers
// ============================================================================

void cover_switch_cluster_on_write_attr(zigbee_cover_switch_cluster *cluster,
                                        uint16_t attribute_id) {
    switch (attribute_id) {
    case ZCL_ATTR_COVER_SWITCH_CONFIG_SWITCH_TYPE:
        // Update the switch press action based on new switch type
        if (cluster->switch_type == ZCL_COVER_SWITCH_TYPE_TOGGLE) {
            cluster->present_value = MULTISTATE_STOP;
        }else{
            cluster->present_value = MULTISTATE_RELEASED;
        }
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                                            ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE);
        cover_switch_cluster_store_attrs_to_nv(cluster);
        break;
    case ZCL_ATTR_COVER_SWITCH_CONFIG_COVER_INDEX:
    case ZCL_ATTR_COVER_SWITCH_CONFIG_REVERSAL:
    case ZCL_ATTR_COVER_SWITCH_CONFIG_LOCAL_MODE:
    case ZCL_ATTR_COVER_SWITCH_CONFIG_BINDED_MODE:
        cover_switch_cluster_store_attrs_to_nv(cluster);
        break;
    case ZCL_ATTR_COVER_SWITCH_CONFIG_LONG_PRESS_DUR:
        // Long press duration is shared between open and close buttons
        cluster->close_button->long_press_duration_ms =
            cluster->open_button->long_press_duration_ms;
        cover_switch_cluster_store_attrs_to_nv(cluster);
        break;
    }
}

void cover_switch_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                         uint16_t attribute_id) {
    cover_switch_cluster_on_write_attr(cover_switch_cluster_by_endpoint[endpoint],
                                       attribute_id);
}

// ============================================================================
// Initialization
// ============================================================================

void cover_switch_cluster_init(zigbee_cover_switch_cluster *cluster) {
    // Attributes
    cluster->switch_type   = ZCL_COVER_SWITCH_TYPE_MOMENTARY;
    cluster->cover_index   = cluster->cover_switch_idx + 1;
    cluster->reversal      = 0;
    cluster->local_mode    = ZCL_COVER_SWITCH_MODE_HYBRID;
    cluster->binded_mode   = ZCL_COVER_SWITCH_MODE_HYBRID;
    cluster->present_value = MULTISTATE_RELEASED;

    // State
    cluster->binded_moving = ZCL_ATTR_WINDOW_COVERING_MOVING_STOPPED;
}

void cover_switch_cluster_add_to_endpoint(zigbee_cover_switch_cluster *cluster,
                                          hal_zigbee_endpoint *endpoint) {
    cover_switch_cluster_by_endpoint[endpoint->endpoint] = cluster;
    cluster->endpoint = endpoint->endpoint;
    cover_switch_cluster_init(cluster);
    cover_switch_cluster_load_attrs_from_nv(cluster);

    cluster->open_button->on_press =
        (ev_button_callback_t)cover_switch_cluster_on_button_press;
    cluster->open_button->on_release =
        (ev_button_callback_t)cover_switch_cluster_on_button_release;
    cluster->open_button->on_long_press =
        (ev_button_callback_t)cover_switch_cluster_on_button_long_press;
    cluster->open_button->callback_param = cluster;

    cluster->close_button->on_press =
        (ev_button_callback_t)cover_switch_cluster_on_button_press;
    cluster->close_button->on_release =
        (ev_button_callback_t)cover_switch_cluster_on_button_release;
    cluster->close_button->on_long_press =
        (ev_button_callback_t)cover_switch_cluster_on_button_long_press;
    cluster->close_button->callback_param = cluster;

    // Configuration attributes on CoverSwitchConfig SERVER cluster (manufacturer-specific)
    SETUP_ATTR_FOR_TABLE(cluster->config_attr_infos, 0,
                         ZCL_ATTR_COVER_SWITCH_CONFIG_SWITCH_TYPE, ZCL_DATA_TYPE_ENUM8,
                         ATTR_WRITABLE, cluster->switch_type);
    SETUP_ATTR_FOR_TABLE(cluster->config_attr_infos, 1,
                         ZCL_ATTR_COVER_SWITCH_CONFIG_COVER_INDEX, ZCL_DATA_TYPE_UINT8,
                         ATTR_WRITABLE, cluster->cover_index);
    SETUP_ATTR_FOR_TABLE(cluster->config_attr_infos, 2,
                         ZCL_ATTR_COVER_SWITCH_CONFIG_REVERSAL, ZCL_DATA_TYPE_BOOLEAN,
                         ATTR_WRITABLE, cluster->reversal);
    SETUP_ATTR_FOR_TABLE(cluster->config_attr_infos, 3,
                         ZCL_ATTR_COVER_SWITCH_CONFIG_LOCAL_MODE, ZCL_DATA_TYPE_ENUM8,
                         ATTR_WRITABLE, cluster->local_mode);
    SETUP_ATTR_FOR_TABLE(cluster->config_attr_infos, 4,
                         ZCL_ATTR_COVER_SWITCH_CONFIG_BINDED_MODE, ZCL_DATA_TYPE_ENUM8,
                         ATTR_WRITABLE, cluster->binded_mode);
    SETUP_ATTR_FOR_TABLE(cluster->config_attr_infos, 5,
                         ZCL_ATTR_COVER_SWITCH_CONFIG_LONG_PRESS_DUR, ZCL_DATA_TYPE_UINT16,
                         ATTR_WRITABLE, cluster->open_button->long_press_duration_ms);

    // CoverSwitchConfig SERVER cluster (manufacturer-specific)
    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_COVER_SWITCH_CONFIG;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 6;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->config_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->cluster_count++;

    // WindowCovering CLIENT cluster (for binding to other devices)
    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_WINDOW_COVERING;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
    endpoint->clusters[endpoint->cluster_count].attributes      = NULL;
    endpoint->clusters[endpoint->cluster_count].is_server       = 0; // CLIENT
    endpoint->cluster_count++;

    // MultiStateInput for button press action reporting
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 0,
                         ZCL_ATTR_MULTISTATE_INPUT_NUMBER_OF_STATES, ZCL_DATA_TYPE_UINT16,
                         ATTR_READONLY, multistate_num_of_states);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 1,
                         ZCL_ATTR_MULTISTATE_INPUT_OUT_OF_SERVICE, ZCL_DATA_TYPE_BOOLEAN,
                         ATTR_READONLY, multistate_out_of_service);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 2,
                         ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE, ZCL_DATA_TYPE_UINT16,
                         ATTR_READONLY, cluster->present_value);
    SETUP_ATTR_FOR_TABLE(cluster->multistate_attr_infos, 3,
                         ZCL_ATTR_MULTISTATE_INPUT_STATUS_FLAGS, ZCL_DATA_TYPE_BITMAP8,
                         ATTR_READONLY, multistate_flags);

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_MULTISTATE_INPUT_BASIC;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 4;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->multistate_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->cluster_count++;
}
