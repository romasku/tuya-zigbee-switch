#ifdef END_DEVICE

#include "poll_control_cluster.h"

#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include "hal/zigbee.h"

// Battery device defaults (ZCL spec)
#define BATTERY_CHECK_IN_INTERVAL          0x3840 // 1 hour in quarter-seconds
#define BATTERY_LONG_POLL_INTERVAL         0x14   // 5 seconds
#define BATTERY_SHORT_POLL_INTERVAL        0x02   // 500ms
#define BATTERY_FAST_POLL_TIMEOUT          0x28   // 10 seconds

// Non-battery device defaults
#define NON_BATTERY_CHECK_IN_INTERVAL      0x3840 // 1 hour
#define NON_BATTERY_LONG_POLL_INTERVAL     0x01   // 250ms (same as short)
#define NON_BATTERY_SHORT_POLL_INTERVAL    0x01   // 250ms
#define NON_BATTERY_FAST_POLL_TIMEOUT      0x28   // 10 seconds

#define QS_TO_MS(qs)    ((uint32_t)(qs) * 250)

static const uint16_t poll_ctrl_cluster_revision = 0x01;

// Single instance pointer for trampoline
static zigbee_poll_control_cluster *poll_ctrl_instance = NULL;

// NVM persistence
typedef struct {
    uint32_t check_in_interval;
    uint32_t long_poll_interval;
    uint16_t short_poll_interval;
    uint16_t fast_poll_timeout;
} poll_control_nv_config;

static poll_control_nv_config nv_config_buffer;

static void poll_control_store_to_nv(zigbee_poll_control_cluster *cluster) {
    nv_config_buffer.check_in_interval   = cluster->check_in_interval;
    nv_config_buffer.long_poll_interval  = cluster->long_poll_interval;
    nv_config_buffer.short_poll_interval = cluster->short_poll_interval;
    nv_config_buffer.fast_poll_timeout   = cluster->fast_poll_timeout;

    hal_nvm_write(NV_ITEM_POLL_CONTROL_CONFIG,
                  sizeof(poll_control_nv_config),
                  (uint8_t *)&nv_config_buffer);
}

static void poll_control_load_from_nv(zigbee_poll_control_cluster *cluster) {
    hal_nvm_status_t st = hal_nvm_read(
        NV_ITEM_POLL_CONTROL_CONFIG,
        sizeof(poll_control_nv_config), (uint8_t *)&nv_config_buffer);

    if (st != HAL_NVM_SUCCESS)
        return;

    cluster->check_in_interval   = nv_config_buffer.check_in_interval;
    cluster->long_poll_interval  = nv_config_buffer.long_poll_interval;
    cluster->short_poll_interval = nv_config_buffer.short_poll_interval;
    cluster->fast_poll_timeout   = nv_config_buffer.fast_poll_timeout;
}

// Fast poll mode management

static void enter_fast_poll(zigbee_poll_control_cluster *cluster,
                            uint16_t timeout_qs) {
    if (timeout_qs == 0) {
        timeout_qs = cluster->fast_poll_timeout;
    }
    cluster->in_fast_poll     = true;
    cluster->fast_poll_end_ms = hal_millis() + QS_TO_MS(timeout_qs);
    hal_zigbee_set_poll_rate_ms(QS_TO_MS(cluster->short_poll_interval));
}

static void exit_fast_poll(zigbee_poll_control_cluster *cluster) {
    cluster->in_fast_poll = false;
    hal_zigbee_set_poll_rate_ms(QS_TO_MS(cluster->long_poll_interval));
}

// ZCL activity callback
static void on_zcl_activity(void) {
    if (poll_ctrl_instance == NULL)
        return;

    printf("ZCL activity, entering fast poll\r\n");
    enter_fast_poll(poll_ctrl_instance, poll_ctrl_instance->fast_poll_timeout);
}

// Check-in timer handler
static void check_in_handler(void *arg) {
    zigbee_poll_control_cluster *cluster = (zigbee_poll_control_cluster *)arg;

    if (cluster->check_in_interval == 0) {
        // Poll control disabled, don't check in
        return;
    }

    printf("Poll control: sending check-in\r\n");

    hal_zigbee_cmd cmd = {
        .endpoint            = cluster->endpoint,
        .profile_id          = ZCL_HA_PROFILE,
        .cluster_id          = ZCL_CLUSTER_POLL_CONTROL,
        .command_id          = ZCL_CMD_POLL_CTRL_CHECK_IN,
        .cluster_specific    =                               1,
        .direction           = HAL_ZIGBEE_DIR_SERVER_TO_CLIENT,
        .disable_default_rsp =                               0,
        .manufacturer_code   =                               0,
        .payload             = NULL,
        .payload_len         =                               0,
    };
    hal_zigbee_send_cmd_to_bindings(&cmd);

    // Enter fast poll to be responsive to check-in response
    enter_fast_poll(cluster, cluster->fast_poll_timeout);

    // Schedule next check-in
    hal_tasks_schedule(&cluster->check_in_task,
                       QS_TO_MS(cluster->check_in_interval));
}

// Command handler

static hal_zigbee_cmd_result_t poll_control_cmd_callback(
    zigbee_poll_control_cluster *cluster, uint8_t command_id,
    void *cmd_payload) {
    uint8_t *data = (uint8_t *)cmd_payload;

    switch (command_id) {
    case ZCL_CMD_POLL_CTRL_CHECK_IN_RSP: {
        if (data == NULL)
            return HAL_ZIGBEE_CMD_SKIPPED;

        bool     start_fast_polling = data[0];
        uint16_t timeout            = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
        printf("Poll control: check-in response, fast=%d, timeout=%d\r\n",
               start_fast_polling, timeout);
        if (start_fast_polling) {
            enter_fast_poll(cluster, timeout);
        } else {
            exit_fast_poll(cluster);
        }
        break;
    }

    case ZCL_CMD_POLL_CTRL_FAST_POLL_STOP:
        printf("Poll control: fast poll stop\r\n");
        exit_fast_poll(cluster);
        break;

    case ZCL_CMD_POLL_CTRL_SET_LONG_POLL_INTERVAL: {
        if (data == NULL)
            return HAL_ZIGBEE_CMD_SKIPPED;

        uint32_t new_interval = (uint32_t)data[0] |
                                ((uint32_t)data[1] << 8) |
                                ((uint32_t)data[2] << 16) |
                                ((uint32_t)data[3] << 24);
        printf("Poll control: set long poll interval=%lu\r\n",
               (unsigned long)new_interval);
        if (new_interval < 0x04 || new_interval > 0x6E0000)
            return HAL_ZIGBEE_CMD_SKIPPED;

        cluster->long_poll_interval = new_interval;
        if (!cluster->in_fast_poll) {
            hal_zigbee_set_poll_rate_ms(QS_TO_MS(new_interval));
        }
        poll_control_store_to_nv(cluster);
        break;
    }

    case ZCL_CMD_POLL_CTRL_SET_SHORT_POLL_INTERVAL: {
        if (data == NULL)
            return HAL_ZIGBEE_CMD_SKIPPED;

        uint16_t new_interval = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
        printf("Poll control: set short poll interval=%d\r\n", new_interval);
        if (new_interval < 0x01)
            return HAL_ZIGBEE_CMD_SKIPPED;

        cluster->short_poll_interval = new_interval;
        if (cluster->in_fast_poll) {
            hal_zigbee_set_poll_rate_ms(QS_TO_MS(new_interval));
        }
        poll_control_store_to_nv(cluster);
        break;
    }

    default:
        return HAL_ZIGBEE_CMD_SKIPPED;
    }

    return HAL_ZIGBEE_CMD_PROCESSED;
}

static hal_zigbee_cmd_result_t poll_control_cmd_trampoline(
    uint8_t endpoint, uint16_t cluster_id, uint8_t command_id,
    void *cmd_payload) {
    if (poll_ctrl_instance == NULL)
        return HAL_ZIGBEE_CMD_SKIPPED;

    return poll_control_cmd_callback(poll_ctrl_instance, command_id,
                                     cmd_payload);
}

// Attribute write handler

void poll_control_cluster_callback_attr_write(uint16_t attribute_id) {
    if (poll_ctrl_instance == NULL)
        return;

    zigbee_poll_control_cluster *cluster = poll_ctrl_instance;

    if (attribute_id == ZCL_ATTR_POLL_CTRL_CHECK_IN_INTERVAL) {
        printf("Poll control: check-in interval written=%lu\r\n",
               (unsigned long)cluster->check_in_interval);
        // Reschedule check-in timer
        hal_tasks_unschedule(&cluster->check_in_task);
        if (cluster->check_in_interval != 0) {
            hal_tasks_schedule(&cluster->check_in_task,
                               QS_TO_MS(cluster->check_in_interval));
        }
        poll_control_store_to_nv(cluster);
    } else if (attribute_id == ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT) {
        printf("Poll control: fast poll timeout written=%d\r\n",
               cluster->fast_poll_timeout);
        poll_control_store_to_nv(cluster);
    }
}

// Public API

void poll_control_cluster_add_to_endpoint(zigbee_poll_control_cluster *cluster,
                                          hal_zigbee_endpoint *endpoint,
                                          bool is_battery_device) {
    poll_ctrl_instance        = cluster;
    cluster->endpoint         = endpoint->endpoint;
    cluster->in_fast_poll     = false;
    cluster->cluster_revision = 1;

    // Set defaults based on device type
    if (is_battery_device) {
        cluster->check_in_interval   = BATTERY_CHECK_IN_INTERVAL;
        cluster->long_poll_interval  = BATTERY_LONG_POLL_INTERVAL;
        cluster->short_poll_interval = BATTERY_SHORT_POLL_INTERVAL;
        cluster->fast_poll_timeout   = BATTERY_FAST_POLL_TIMEOUT;
    } else {
        cluster->check_in_interval   = NON_BATTERY_CHECK_IN_INTERVAL;
        cluster->long_poll_interval  = NON_BATTERY_LONG_POLL_INTERVAL;
        cluster->short_poll_interval = NON_BATTERY_SHORT_POLL_INTERVAL;
        cluster->fast_poll_timeout   = NON_BATTERY_FAST_POLL_TIMEOUT;
    }

    // Load overrides from NVM
    poll_control_load_from_nv(cluster);

    // Setup attributes
    SETUP_ATTR(0, ZCL_ATTR_POLL_CTRL_CHECK_IN_INTERVAL, ZCL_DATA_TYPE_UINT32,
               ATTR_WRITABLE, cluster->check_in_interval);
    SETUP_ATTR(1, ZCL_ATTR_POLL_CTRL_LONG_POLL_INTERVAL, ZCL_DATA_TYPE_UINT32,
               ATTR_READONLY, cluster->long_poll_interval);
    SETUP_ATTR(2, ZCL_ATTR_POLL_CTRL_SHORT_POLL_INTERVAL,
               ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->short_poll_interval);
    SETUP_ATTR(3, ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT, ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE, cluster->fast_poll_timeout);
    SETUP_ATTR(4, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, cluster->cluster_revision);

    // Register cluster on endpoint
    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_POLL_CONTROL;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 5;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    =
        poll_control_cmd_trampoline;
    endpoint->cluster_count++;

    // Register ZCL activity callback
    hal_zigbee_register_on_zcl_activity_callback(on_zcl_activity);

    // Start on short poll
    hal_zigbee_set_poll_rate_ms(QS_TO_MS(cluster->short_poll_interval));
    // Mark as in fast poll so we transition to long poll after timeout
    enter_fast_poll(cluster, cluster->fast_poll_timeout);

    // Schedule check-in timer
    cluster->check_in_task.handler = (task_handler_t)check_in_handler;
    cluster->check_in_task.arg     = cluster;
    hal_tasks_init(&cluster->check_in_task);
    if (cluster->check_in_interval != 0) {
        hal_tasks_schedule(&cluster->check_in_task,
                           QS_TO_MS(cluster->check_in_interval));
    }
}

void poll_control_cluster_update(void) {
    if (poll_ctrl_instance == NULL)
        return;

    zigbee_poll_control_cluster *cluster = poll_ctrl_instance;
    if (cluster->in_fast_poll &&
        (int32_t)(hal_millis() - cluster->fast_poll_end_ms) >= 0) {
        printf("Poll control: fast poll timeout, switching to long poll\r\n");
        exit_fast_poll(cluster);
    }
}

#endif // END_DEVICE
