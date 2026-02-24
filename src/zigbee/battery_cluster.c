#include "battery_cluster.h"

#ifdef BATTERY_POWERED

#include "cluster_common.h"
#include "consts.h"
#include "hal/battery.h"
#include "hal/timer.h"

const uint16_t battery_cluster_revision = 0x01;

static zigbee_battery_cluster *g_battery_cluster = NULL;
static uint32_t last_update_ms = 0;
#define BATTERY_UPDATE_THROTTLE_MS 5000

static void battery_cluster_notify_if_changed(zigbee_battery_cluster *cluster,
                                              uint16_t attr_id,
                                              uint8_t *stored, uint8_t new_val) {
    if (*stored == new_val) {
        return;
    }
    *stored = new_val;
    hal_zigbee_set_attribute_value(cluster->endpoint, ZCL_CLUSTER_POWER_CFG,
                                   attr_id, stored);
    hal_zigbee_notify_attribute_changed(cluster->endpoint, ZCL_CLUSTER_POWER_CFG,
                                        attr_id);
}

void battery_cluster_add_to_endpoint(zigbee_battery_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint) {
    g_battery_cluster = cluster;
    cluster->endpoint = endpoint->endpoint;

    cluster->percentage_remaining = 200; // 100% in ZCL (0.5% steps)
    cluster->voltage_100mv = 0xFF;       // Unknown at init

    SETUP_ATTR(0, ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE, ZCL_DATA_TYPE_UINT8,
               ATTR_READONLY, cluster->voltage_100mv);
    SETUP_ATTR(1, ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE, ZCL_DATA_TYPE_UINT8,
               ATTR_READONLY, cluster->percentage_remaining);
    SETUP_ATTR(2, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, battery_cluster_revision);

    endpoint->clusters[endpoint->cluster_count].cluster_id = ZCL_CLUSTER_POWER_CFG;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 3;
    endpoint->clusters[endpoint->cluster_count].attributes = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback = NULL;
    endpoint->cluster_count++;

    // Initial battery reading (bypass throttle since last_update_ms is 0 at boot)
    last_update_ms = 0;
    battery_cluster_update(cluster);
}

void battery_cluster_update(zigbee_battery_cluster *cluster) {
    uint32_t now = hal_millis();
    if (now - last_update_ms < BATTERY_UPDATE_THROTTLE_MS) {
        return;
    }
    last_update_ms = now;

    uint8_t percentage = hal_battery_get_percentage();
    uint16_t voltage_mv = hal_battery_get_voltage_mv();

    uint8_t zcl_percentage = (percentage > 100) ? 200 : (percentage * 2);
    uint8_t voltage_100mv;
    if (voltage_mv == 0) {
        voltage_100mv = 0xFF;
    } else {
        uint16_t rounded = (voltage_mv + 50) / 100;
        voltage_100mv = (rounded > 0xFF) ? 0xFF : (uint8_t)rounded;
    }

    battery_cluster_notify_if_changed(cluster, ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE,
                                      &cluster->percentage_remaining, zcl_percentage);
    battery_cluster_notify_if_changed(cluster, ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE,
                                      &cluster->voltage_100mv, voltage_100mv);
}

void battery_cluster_update_on_event(void) {
    if (g_battery_cluster != NULL) {
        battery_cluster_update(g_battery_cluster);
    }
}

void battery_cluster_force_report(void) {
    if (g_battery_cluster == NULL) {
        return;
    }

    last_update_ms = 0;
    battery_cluster_update(g_battery_cluster);

    hal_zigbee_notify_attribute_changed(g_battery_cluster->endpoint,
                                        ZCL_CLUSTER_POWER_CFG,
                                        ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE);
    hal_zigbee_notify_attribute_changed(g_battery_cluster->endpoint,
                                        ZCL_CLUSTER_POWER_CFG,
                                        ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE);
}

#endif // BATTERY_POWERED
