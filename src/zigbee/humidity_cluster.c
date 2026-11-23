#include "humidity_cluster.h"

#include "cluster_common.h"
#include "consts.h"
#include "hal/zigbee.h"

#if HAS_ONEWIRE

static uint16_t humidity_min_value        = 0;
static uint16_t humidity_max_value        = 10000;
static uint16_t humidity_cluster_revision = 0x0001;

zigbee_humidity_cluster humidity_clusters[OW_MAX_SENSORS];
uint8_t humidity_clusters_cnt = 0;

void humidity_cluster_add_to_endpoint(zigbee_humidity_cluster *cluster,
                                      hal_zigbee_endpoint *endpoint) {
    cluster->endpoint       = endpoint->endpoint;
    cluster->measured_value = ZCL_HUMIDITY_INVALID;
    endpoint->device_id     = ZCL_HA_DEVICEID_TEMPERATURE_SENSOR;

    SETUP_ATTR(0, ZCL_ATTR_HUMIDITY_MEASURED_VALUE, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, cluster->measured_value);
    SETUP_ATTR(1, ZCL_ATTR_HUMIDITY_MIN_MEASURED_VALUE, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, humidity_min_value);
    SETUP_ATTR(2, ZCL_ATTR_HUMIDITY_MAX_MEASURED_VALUE, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, humidity_max_value);
    SETUP_ATTR(3, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, humidity_cluster_revision);

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_RELATIVE_HUMIDITY;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 4;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    = NULL;
    endpoint->cluster_count++;
}

void humidity_cluster_publish(const uint16_t rh_percent_x100[OW_MAX_SENSORS],
                              uint8_t force) {
    for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
        if (humidity_clusters[i].endpoint == 0) {
            continue;
        }
        if (!force &&
            humidity_clusters[i].measured_value == rh_percent_x100[i]) {
            continue;
        }
        humidity_clusters[i].measured_value = rh_percent_x100[i];
        hal_zigbee_notify_attribute_changed(humidity_clusters[i].endpoint,
                                            ZCL_CLUSTER_RELATIVE_HUMIDITY,
                                            ZCL_ATTR_HUMIDITY_MEASURED_VALUE);
        hal_zigbee_send_report_attr(humidity_clusters[i].endpoint,
                                    ZCL_CLUSTER_RELATIVE_HUMIDITY,
                                    ZCL_ATTR_HUMIDITY_MEASURED_VALUE,
                                    ZCL_DATA_TYPE_UINT16,
                                    &humidity_clusters[i].measured_value,
                                    sizeof(humidity_clusters[i].measured_value));
    }
}

#endif
