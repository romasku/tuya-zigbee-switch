#include "battery_cluster.h"

#include "cluster_common.h"
#include "consts.h"
#include "device_config/config_parser.h"
#include "base_components/battery.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include "hal/zigbee.h"

const uint16_t battery_cluster_revision = 0x01;

static zigbee_battery_cluster *g_battery_cluster = NULL;
static uint32_t last_update_ms = 0;
#define BATTERY_UPDATE_THROTTLE_MS    5000

void battery_cluster_add_to_endpoint(zigbee_battery_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint) {
    g_battery_cluster = cluster;
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

    // Initial battery reading (bypass throttle since last_update_ms is 0 at boot)
    last_update_ms = 0;
    battery_cluster_update(cluster);
    // No periodic timer — battery is checked on each poll wakeup
    // via battery_cluster_update_on_event() in app_reinit_retention().
}

void battery_cluster_update(zigbee_battery_cluster *cluster) {
    uint32_t now = hal_millis();

    if (now - last_update_ms < BATTERY_UPDATE_THROTTLE_MS) {
        return;
    }
    last_update_ms = now;

    cluster->percentage_remaining = battery_get_charge(&battery, 100);
    cluster->voltage_100mv        = battery_get_mv(&battery) / 100;

    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_POWER_CFG,
                                        ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE);
    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_POWER_CFG,
                                        ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE);
}

void battery_cluster_update_on_event(void) {
    if (g_battery_cluster != NULL) {
        battery_cluster_update(g_battery_cluster);
    }
}
