#include "temperature_cluster.h"

#include "cluster_common.h"
#include "consts.h"
#include "hal/printf_selector.h"
#include "hal/zigbee.h"
#include "humidity_cluster.h"

#include <string.h>

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
    /* Drain temperature, humidity, and id reports one packet at a time. */
    ONEWIRE_REPORT,
};

static uint8_t onewire_phase;
/* A full report pass has completed, so later passes send only changes. */
static uint8_t snapshot_sent;
static uint8_t slot_limit_logged;

/* The outgoing cache is 16 packets. A boot snapshot is 24 reports, so humidity
 * was queued after every temperature and then dropped. One report per step. */
#define REPORT_PACE_MS            50

enum {
    REPORT_TEMPERATURE = 0,
    REPORT_HUMIDITY,
    REPORT_ID,
};

static uint8_t report_slot;
static uint8_t report_part;
static uint8_t report_open;
static int16_t sent_temp[OW_MAX_SENSORS];
static uint16_t sent_humidity[OW_MAX_SENSORS];
static uint8_t sent_id_len[OW_MAX_SENSORS];
static uint8_t sent_id_text[OW_MAX_SENSORS][23];
static uint8_t sent_temp_ok[OW_MAX_SENSORS];
static uint8_t sent_humidity_ok[OW_MAX_SENSORS];
static uint8_t sent_id_ok[OW_MAX_SENSORS];

static uint8_t report_needs_retry(hal_zigbee_status_t status) {
    return status == HAL_ZIGBEE_ERR_NOT_JOINED ||
           status == HAL_ZIGBEE_ERR_SEND_FAILED;
}

static void note_endpoint_limit(uint8_t slot) {
    uint8_t seen = ow_seen_mask();
    if ((seen & (1u << slot)) && (slot_limit_logged & (1u << slot)) == 0 &&
        temperature_clusters[slot].endpoint == 0) {
        slot_limit_logged |= (uint8_t)(1u << slot);
        printf("1-Wire slot %d left off the network, endpoint limit\r\n", slot);
    }
}

/* 0: try this item again. 1: one report was handed to the stack, or there
 * was nothing to send. 2: the pass is finished. */
static uint8_t report_step(void) {
    for (;;) {
        if (report_slot >= OW_MAX_SENSORS) {
            return 2;
        }
        uint8_t slot = report_slot;
        note_endpoint_limit(slot);
        if (temperature_clusters[slot].endpoint == 0 &&
            humidity_clusters[slot].endpoint == 0) {
            report_slot++;
            report_part = REPORT_TEMPERATURE;
            continue;
        }

        if (report_part == REPORT_TEMPERATURE) {
            if (temperature_clusters[slot].endpoint == 0) {
                report_part = REPORT_HUMIDITY;
                continue;
            }
            if (snapshot_sent && sent_temp_ok[slot] &&
                sent_temp[slot] == read_temps[slot]) {
                report_part = REPORT_HUMIDITY;
                continue;
            }
            temperature_clusters[slot].measured_value = read_temps[slot];
            hal_zigbee_status_t status = hal_zigbee_send_report_attr(
                temperature_clusters[slot].endpoint,
                ZCL_CLUSTER_TEMPERATURE_MEASUREMENT,
                ZCL_ATTR_TEMPERATURE_MEASURED_VALUE, ZCL_DATA_TYPE_INT16,
                &temperature_clusters[slot].measured_value,
                sizeof(temperature_clusters[slot].measured_value));
            if (report_needs_retry(status)) {
                return 0;
            }
            if (status == HAL_ZIGBEE_OK) {
                sent_temp[slot]    = read_temps[slot];
                sent_temp_ok[slot] = 1;
            }
            report_part = REPORT_HUMIDITY;
            return 1;
        }

        if (report_part == REPORT_HUMIDITY) {
            if (humidity_clusters[slot].endpoint == 0) {
                report_part = REPORT_ID;
                continue;
            }
            if (snapshot_sent && sent_humidity_ok[slot] &&
                sent_humidity[slot] == read_humidity[slot]) {
                report_part = REPORT_ID;
                continue;
            }
            humidity_clusters[slot].measured_value = read_humidity[slot];
            hal_zigbee_status_t status = hal_zigbee_send_report_attr(
                humidity_clusters[slot].endpoint, ZCL_CLUSTER_RELATIVE_HUMIDITY,
                ZCL_ATTR_HUMIDITY_MEASURED_VALUE, ZCL_DATA_TYPE_UINT16,
                &humidity_clusters[slot].measured_value,
                sizeof(humidity_clusters[slot].measured_value));
            if (report_needs_retry(status)) {
                return 0;
            }
            if (status == HAL_ZIGBEE_OK) {
                sent_humidity[slot]    = read_humidity[slot];
                sent_humidity_ok[slot] = 1;
            }
            report_part = REPORT_ID;
            return 1;
        }

        if (temperature_clusters[slot].endpoint == 0) {
            report_slot++;
            report_part = REPORT_TEMPERATURE;
            continue;
        }
        char    text[24];
        uint8_t connected = ow_slot_id(slot, text);
        uint8_t len       = 0;
        if (connected) {
            len = (uint8_t)strlen(text);
            if (len > 23) {
                len = 23;
            }
        }
        uint8_t same = sent_id_ok[slot] && sent_id_len[slot] == len &&
                       (len == 0 || memcmp(sent_id_text[slot], text, len) == 0);
        if (snapshot_sent && same) {
            report_slot++;
            report_part = REPORT_TEMPERATURE;
            continue;
        }
        /* An empty id is sent on the boot pass so a retained id is cleared.
         * Later, it is sent only when a previously reported id goes away. */
        if (snapshot_sent && len == 0 && !sent_id_ok[slot]) {
            report_slot++;
            report_part = REPORT_TEMPERATURE;
            continue;
        }
        zigbee_temperature_cluster *cluster = &temperature_clusters[slot];
        cluster->sensor_id[0] = len;
        if (len != 0) {
            memcpy(&cluster->sensor_id[1], text, len);
        }
        hal_zigbee_status_t status = hal_zigbee_send_report_attr(
            cluster->endpoint, ZCL_CLUSTER_TEMPERATURE_MEASUREMENT,
            ZCL_ATTR_TEMPERATURE_SENSOR_ID, ZCL_DATA_TYPE_CHAR_STR,
            cluster->sensor_id, (uint8_t)(cluster->sensor_id[0] + 1));
        if (report_needs_retry(status)) {
            return 0;
        }
        if (status == HAL_ZIGBEE_OK) {
            sent_id_len[slot] = len;
            if (len != 0) {
                memcpy(sent_id_text[slot], text, len);
            }
            sent_id_ok[slot] = 1;
        }
        report_slot++;
        report_part = REPORT_TEMPERATURE;
        return 1;
    }
}

static void start_report_pass(void) {
    if (report_open) {
        return;
    }
    report_slot = 0;
    report_part = REPORT_TEMPERATURE;
    report_open = 1;
    onewire_phase = ONEWIRE_REPORT;
    hal_tasks_schedule(&temperature_task, REPORT_PACE_MS);
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

static void temperature_task_handler(void *arg) {
    (void)arg;

    if (onewire_phase == ONEWIRE_REPORT) {
        uint8_t step = report_step();
        if (step == 2) {
            report_open    = 0;
            snapshot_sent  = 1;
            if (ow_sensors_seen()) {
                schedule_measurements();
            }
            return;
        }
        hal_tasks_schedule(&temperature_task, REPORT_PACE_MS);
        return;
    }

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
    log_seen_slots();
    start_report_pass();
}

void temperature_cluster_add_to_endpoint(zigbee_temperature_cluster *cluster,
                                        hal_zigbee_endpoint *endpoint) {
    cluster->endpoint         = endpoint->endpoint;
    cluster->measured_value   = ZCL_TEMPERATURE_INVALID;
    cluster->sensor_id[0]     = 0;
    endpoint->device_id       = ZCL_HA_DEVICEID_TEMPERATURE_SENSOR;

    SETUP_ATTR(0, ZCL_ATTR_TEMPERATURE_MEASURED_VALUE, ZCL_DATA_TYPE_INT16,
               ATTR_READONLY, cluster->measured_value);
    SETUP_ATTR(1, ZCL_ATTR_TEMPERATURE_MIN_MEASURED_VALUE, ZCL_DATA_TYPE_INT16,
               ATTR_READONLY, temperature_min_value);
    SETUP_ATTR(2, ZCL_ATTR_TEMPERATURE_MAX_MEASURED_VALUE, ZCL_DATA_TYPE_INT16,
               ATTR_READONLY, temperature_max_value);
    SETUP_ATTR(3, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, temperature_cluster_revision);
    SETUP_ATTR(4, ZCL_ATTR_TEMPERATURE_SENSOR_ID, ZCL_DATA_TYPE_CHAR_STR,
               ATTR_READONLY, cluster->sensor_id);

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_TEMPERATURE_MEASUREMENT;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 5;
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
        for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
            read_temps[i]    = ZCL_TEMPERATURE_INVALID;
            read_humidity[i] = ZCL_HUMIDITY_INVALID;
        }
        start_report_pass();
        return;
    }
    start_measurement();
}