#include "temperature_cluster.h"

#include "cluster_common.h"
#include "consts.h"
#include "device_config/config_parser.h"
#include "hal/printf_selector.h"
#include "hal/zigbee.h"
#include "humidity_cluster.h"

static int16_t  temperature_min_value = -5500;
static int16_t  temperature_max_value = 12500;
static uint16_t temperature_cluster_revision = 0x0001;

#define TEMPERATURE_CONVERT_MS    800
/* Time from one published sample to the next, including Convert. */
#define TEMPERATURE_PERIOD_MS     5000
/*
 * One sensor read, then return. The main loop clears the 1000 ms watchdog
 * around this task, so a step has to stay well under that. The boot search
 * itself runs inline in dallas1wire_clusters_start, before the watchdog.
 */
#define ONEWIRE_STEP_MS           20

zigbee_temperature_cluster temperature_clusters[OW_MAX_SENSORS];
uint8_t temperature_clusters_cnt = 0;

static hal_task_t temperature_task;
static int16_t    read_temps[OW_MAX_SENSORS];
static uint16_t   read_humidity[OW_MAX_SENSORS];
static uint8_t    powerup_waits;

enum {
    ONEWIRE_READ = 0,
    ONEWIRE_MEASURE,
};

static uint8_t onewire_phase;

static void publish_temperatures(const int16_t temps[OW_MAX_SENSORS]) {
    for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
        if (temperature_clusters[i].endpoint == 0 ||
            temperature_clusters[i].measured_value == temps[i]) {
            continue;
        }
        temperature_clusters[i].measured_value = temps[i];
        hal_zigbee_notify_attribute_changed(
            temperature_clusters[i].endpoint,
            ZCL_CLUSTER_TEMPERATURE_MEASUREMENT,
            ZCL_ATTR_TEMPERATURE_MEASURED_VALUE);
    }
}

static void begin_read(void) {
    ow_read_begin(read_temps, read_humidity);
    onewire_phase = ONEWIRE_READ;
    hal_tasks_schedule(&temperature_task, ONEWIRE_STEP_MS);
}

static uint8_t reading_is_powerup(void) {
    for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
        /* 85.00°C is the DS18B20 scratchpad value before Convert. */
        if (read_temps[i] == 8500) {
            return 1;
        }
    }
    return 0;
}

static void start_measurement(void) {
    if (!ow_sensors_seen()) {
        return;
    }
    /*
     * Convert, then read. This ATtiny leaves 85.00°C in the scratchpad
     * until Convert finishes, and that is the sample the log shows.
     */
    if (ow_convert_start()) {
        ow_read_begin(read_temps, read_humidity);
        onewire_phase = ONEWIRE_READ;
        hal_tasks_schedule(&temperature_task, TEMPERATURE_CONVERT_MS);
        return;
    }
    begin_read();
}

static void schedule_measurements(void) {
    onewire_phase = ONEWIRE_MEASURE;
    /* Convert runs at the start of the next sample, so it counts toward the 5 s. */
    hal_tasks_schedule(&temperature_task,
                       TEMPERATURE_PERIOD_MS - TEMPERATURE_CONVERT_MS);
}

static uint8_t seen_count(void) {
    uint8_t seen  = ow_seen_mask();
    uint8_t count = 0;
    for (uint8_t slot = 0; slot < OW_MAX_SENSORS; slot++) {
        if (seen & (1u << slot)) {
            count++;
        }
    }
    return count;
}

static void log_seen_slots(void) {
    uint8_t seen = ow_seen_mask();
    for (uint8_t slot = 0; slot < OW_MAX_SENSORS; slot++) {
        if (seen & (1u << slot)) {
            ow_log_slot(slot, temperature_clusters[slot].endpoint);
        }
    }
}

static void attach_seen_slots(void) {
    uint8_t seen = ow_seen_mask();
    for (uint8_t slot = 0; slot < OW_MAX_SENSORS; slot++) {
        if (seen & (1u << slot)) {
            config_attach_onewire_sensor(slot, ow_slot_family(slot));
        }
    }
}

static void temperature_task_handler(void *arg) {
    (void)arg;

    if (onewire_phase == ONEWIRE_MEASURE) {
        start_measurement();
        return;
    }

    if (ow_read_step(read_temps, read_humidity)) {
        hal_tasks_schedule(&temperature_task, ONEWIRE_STEP_MS);
        return;
    }
    if (reading_is_powerup() && powerup_waits < 2) {
        powerup_waits++;
        start_measurement();
        return;
    }
    powerup_waits = 0;
    publish_temperatures(read_temps);
    humidity_cluster_publish(read_humidity);
    log_seen_slots();
    schedule_measurements();
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

void dallas1wire_clusters_start(void) {
    if (!ow_is_configured()) {
        return;
    }
    temperature_task.handler = temperature_task_handler;
    temperature_task.arg     = NULL;
    hal_tasks_init(&temperature_task);

    printf("1-Wire bus initialized, searching once\r\n");
    ow_search_begin();
    while (ow_search_step()) {
    }
    ow_search_alternate_resets();
    printf("1-Wire search done, %d sensor(s)\r\n", seen_count());
    if (!ow_sensors_seen()) {
        return;
    }
    attach_seen_slots();
    start_measurement();
}
