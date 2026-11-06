#include "temperature_cluster.h"

#include "cluster_common.h"
#include "consts.h"
#include "hal/printf_selector.h"
#include "hal/zigbee.h"
#include "humidity_cluster.h"

static int16_t  temperature_min_value = -5500;
static int16_t  temperature_max_value = 12500;
static uint16_t temperature_cluster_revision = 0x0001;

#define TEMPERATURE_CONVERT_MS    800
#define TEMPERATURE_PERIOD_MS     10000

zigbee_temperature_cluster temperature_clusters[DS18B20_MAX_SENSORS];
uint8_t temperature_clusters_cnt = 0;

static hal_task_t temperature_task;
static uint8_t    waiting_for_conversion;

static void publish_temperatures(const int16_t temps[DS18B20_MAX_SENSORS]) {
    for (uint8_t i = 0; i < temperature_clusters_cnt && i < DS18B20_MAX_SENSORS;
         i++) {
        if (temperature_clusters[i].measured_value == temps[i]) {
            continue;
        }
        temperature_clusters[i].measured_value = temps[i];
        hal_zigbee_notify_attribute_changed(
            temperature_clusters[i].endpoint,
            ZCL_CLUSTER_TEMPERATURE_MEASUREMENT,
            ZCL_ATTR_TEMPERATURE_MEASURED_VALUE);
    }
}

static void temperature_task_handler(void *arg) {
    (void)arg;
    if (!waiting_for_conversion) {
        ds18b20_start_conversion();
        waiting_for_conversion = 1;
        hal_tasks_schedule(&temperature_task, TEMPERATURE_CONVERT_MS);
        return;
    }

    int16_t temps[DS18B20_MAX_SENSORS];
    ds18b20_read_temperatures(temps);
    publish_temperatures(temps);

    uint16_t humidity[DS18B20_MAX_SENSORS];
    ds18b20_read_humidities(humidity);
    humidity_cluster_publish(humidity);

    waiting_for_conversion = 0;
    hal_tasks_schedule(&temperature_task, TEMPERATURE_PERIOD_MS);
}

void temperature_cluster_add_to_endpoint(zigbee_temperature_cluster *cluster,
                                        hal_zigbee_endpoint *endpoint) {
    cluster->endpoint        = endpoint->endpoint;
    cluster->measured_value  = ZCL_TEMPERATURE_INVALID;
    endpoint->device_id      = ZCL_HA_DEVICEID_TEMPERATURE_SENSOR;

    SETUP_ATTR(0, ZCL_ATTR_TEMPERATURE_MEASURED_VALUE, ZCL_DATA_TYPE_INT16,
               ATTR_READONLY, cluster->measured_value);
    SETUP_ATTR(1, ZCL_ATTR_TEMPERATURE_MIN_MEASURED_VALUE, ZCL_DATA_TYPE_INT16,
               ATTR_READONLY, temperature_min_value);
    SETUP_ATTR(2, ZCL_ATTR_TEMPERATURE_MAX_MEASURED_VALUE, ZCL_DATA_TYPE_INT16,
               ATTR_READONLY, temperature_max_value);
    SETUP_ATTR(3, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, temperature_cluster_revision);

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_TEMPERATURE_MEASUREMENT;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 4;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback    = NULL;
    endpoint->cluster_count++;
}

void temperature_clusters_start(void) {
    if (!ds18b20_is_configured() ||
        (temperature_clusters_cnt == 0 && humidity_clusters_cnt == 0)) {
        return;
    }
    waiting_for_conversion        = 0;
    temperature_task.handler      = temperature_task_handler;
    temperature_task.arg          = NULL;
    hal_tasks_init(&temperature_task);
    hal_tasks_schedule(&temperature_task, 1000);
    printf("1-Wire bus started, %d sensor endpoints\r\n",
           temperature_clusters_cnt);
}
