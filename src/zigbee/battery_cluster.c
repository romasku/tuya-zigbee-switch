#include "battery_cluster.h"

#include "cluster_common.h"
#include "consts.h"
#include "device_config/config_parser.h"
#include "base_components/battery.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include "hal/zigbee.h"
#include "hal/tasks.h"

const uint16_t battery_cluster_revision = 0x01;

#define BATTERY_REFRESH_INTERVAL_MS    300000   // 5 minutes

void battery_cluster_add_to_endpoint(zigbee_battery_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint) {
    cluster->endpoint = endpoint->endpoint;

    cluster->voltage_100mv        = 30;  // 3.0V default
    cluster->percentage_remaining = 200; // 100% in ZCL (0.5% steps)

    SETUP_ATTR(0, ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE, ZCL_DATA_TYPE_UINT8,
               ATTR_READONLY, cluster->voltage_100mv);
    SETUP_ATTR(1, ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE, ZCL_DATA_TYPE_UINT8,
               ATTR_READONLY, cluster->percentage_remaining);
    SETUP_ATTR(2, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, battery_cluster_revision);

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_POWER_CFG;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 3;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    = NULL;
    endpoint->cluster_count++;

    // Initial battery reading
    cluster->refresh_values_task.handler = (task_handler_t)battery_cluster_update;
    cluster->refresh_values_task.arg     = cluster;
    hal_tasks_init(&cluster->refresh_values_task);

    battery_cluster_update(cluster); // Also schedules next update
}

void battery_cluster_update(zigbee_battery_cluster *cluster) {
    battery_status_t status = battery_get_status(&battery);

    cluster->percentage_remaining = status.charge;
    cluster->voltage_100mv        = status.voltage_mv / 100;

    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_POWER_CFG,
                                        ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE, false);
    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_POWER_CFG,
                                        ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE, false);
    hal_tasks_schedule(&cluster->refresh_values_task, BATTERY_REFRESH_INTERVAL_MS);
}
