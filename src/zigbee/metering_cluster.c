#include "metering_cluster.h"

#include <string.h>

#include "cluster_common.h"
#include "consts.h"
#include "hal/printf_selector.h"

static void metering_cluster_on_driver_update(void *param);

static void set_uint48(uint8_t out[6], uint64_t value) {
    for (int i = 0; i < 6; i++) {
        out[i] = (uint8_t)(value >> (8 * i));
    }
}

void metering_cluster_add_to_endpoint(zigbee_metering_cluster *cluster,
                                      hal_zigbee_endpoint *endpoint) {
    cluster->endpoint = endpoint->endpoint;

    // --- Electrical Measurement (0x0B04) ---
    cluster->measurement_type = 1; // bit0: active measurement (AC)
    cluster->ac_voltage_mult  = 1;
    cluster->ac_voltage_div   = 10;   // raw is deciVolt
    cluster->ac_current_mult  = 1;
    cluster->ac_current_div   = 1000; // raw is mA
    cluster->ac_power_mult    = 1;
    cluster->ac_power_div     = 10;   // raw is deciWatt

    {
        hal_zigbee_attribute *table = cluster->em_attr_infos;
        SETUP_ATTR_FOR_TABLE(table, 0, ZCL_ATTR_EM_MEASUREMENT_TYPE,
                             ZCL_DATA_TYPE_BITMAP32, ATTR_READONLY,
                             cluster->measurement_type);
        SETUP_ATTR_FOR_TABLE(table, 1, ZCL_ATTR_EM_RMS_VOLTAGE,
                             ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                             cluster->metering->voltage_dv);
        SETUP_ATTR_FOR_TABLE(table, 2, ZCL_ATTR_EM_RMS_CURRENT,
                             ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                             cluster->metering->current_ma);
        SETUP_ATTR_FOR_TABLE(table, 3, ZCL_ATTR_EM_ACTIVE_POWER,
                             ZCL_DATA_TYPE_INT16, ATTR_READONLY,
                             cluster->metering->power_dw);
        SETUP_ATTR_FOR_TABLE(table, 4, ZCL_ATTR_EM_AC_VOLTAGE_MULT,
                             ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                             cluster->ac_voltage_mult);
        SETUP_ATTR_FOR_TABLE(table, 5, ZCL_ATTR_EM_AC_VOLTAGE_DIV,
                             ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                             cluster->ac_voltage_div);
        SETUP_ATTR_FOR_TABLE(table, 6, ZCL_ATTR_EM_AC_CURRENT_MULT,
                             ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                             cluster->ac_current_mult);
        SETUP_ATTR_FOR_TABLE(table, 7, ZCL_ATTR_EM_AC_CURRENT_DIV,
                             ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                             cluster->ac_current_div);
        SETUP_ATTR_FOR_TABLE(table, 8, ZCL_ATTR_EM_AC_POWER_MULT,
                             ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                             cluster->ac_power_mult);
        SETUP_ATTR_FOR_TABLE(table, 9, ZCL_ATTR_EM_AC_POWER_DIV,
                             ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
                             cluster->ac_power_div);
    }

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_ELECTRICAL_MEASUREMENT;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 10;
    endpoint->clusters[endpoint->cluster_count].attributes =
        cluster->em_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server    = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback = NULL;
    endpoint->cluster_count++;

    // --- Metering (0x0702) ---
    set_uint48(cluster->curr_summ_delivered, cluster->metering->energy_wh);
    cluster->status               = 0;    // bitmap8, no error
    cluster->unit_of_measure      = 0;    // kWh
    cluster->multiplier           = 1;    // uint24
    cluster->divisor              = 1000; // Wh -> kWh
    cluster->summation_formatting = 0x2B; // 3 decimals, suppress zeros
    cluster->metering_device_type = 0;    // electric metering

    {
        hal_zigbee_attribute *table = cluster->mt_attr_infos;
        SETUP_ATTR_FOR_TABLE(table, 0, ZCL_ATTR_METERING_CURR_SUMM_DELIVERED,
                             ZCL_DATA_TYPE_UINT48, ATTR_READONLY,
                             cluster->curr_summ_delivered);
        SETUP_ATTR_FOR_TABLE(table, 1, ZCL_ATTR_METERING_STATUS,
                             ZCL_DATA_TYPE_BITMAP8, ATTR_READONLY,
                             cluster->status);
        SETUP_ATTR_FOR_TABLE(table, 2, ZCL_ATTR_METERING_UNIT_OF_MEASURE,
                             ZCL_DATA_TYPE_ENUM8, ATTR_READONLY,
                             cluster->unit_of_measure);
        SETUP_ATTR_FOR_TABLE(table, 3, ZCL_ATTR_METERING_MULTIPLIER,
                             ZCL_DATA_TYPE_UINT24, ATTR_READONLY,
                             cluster->multiplier);
        SETUP_ATTR_FOR_TABLE(table, 4, ZCL_ATTR_METERING_DIVISOR,
                             ZCL_DATA_TYPE_UINT24, ATTR_READONLY,
                             cluster->divisor);
        SETUP_ATTR_FOR_TABLE(table, 5, ZCL_ATTR_METERING_SUMM_FORMATTING,
                             ZCL_DATA_TYPE_BITMAP8, ATTR_READONLY,
                             cluster->summation_formatting);
        SETUP_ATTR_FOR_TABLE(table, 6, ZCL_ATTR_METERING_DEVICE_TYPE,
                             ZCL_DATA_TYPE_BITMAP8, ATTR_READONLY,
                             cluster->metering_device_type);
    }
    // uint24 attributes: ZCL wire size is 3 bytes, storage is uint32
    cluster->mt_attr_infos[3].size = 3;
    cluster->mt_attr_infos[4].size = 3;

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_METERING;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 7;
    endpoint->clusters[endpoint->cluster_count].attributes =
        cluster->mt_attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server    = 1;
    endpoint->clusters[endpoint->cluster_count].cmd_callback = NULL;
    endpoint->cluster_count++;

    // Hook driver updates -> attribute change notifications
    cluster->metering->on_update      = metering_cluster_on_driver_update;
    cluster->metering->callback_param = cluster;
}

static void metering_cluster_on_driver_update(void *param) {
    zigbee_metering_cluster *cluster = (zigbee_metering_cluster *)param;
    metering_bl0937_t *      m       = cluster->metering;

    if (m->voltage_dv != cluster->last_voltage_dv) {
        cluster->last_voltage_dv = m->voltage_dv;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_ELECTRICAL_MEASUREMENT,
                                            ZCL_ATTR_EM_RMS_VOLTAGE);
    }
    if (m->current_ma != cluster->last_current_ma) {
        cluster->last_current_ma = m->current_ma;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_ELECTRICAL_MEASUREMENT,
                                            ZCL_ATTR_EM_RMS_CURRENT);
    }
    if (m->power_dw != cluster->last_power_dw) {
        cluster->last_power_dw = m->power_dw;
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_ELECTRICAL_MEASUREMENT,
                                            ZCL_ATTR_EM_ACTIVE_POWER);
    }
    if (m->energy_wh != cluster->last_energy_wh) {
        cluster->last_energy_wh = m->energy_wh;
        set_uint48(cluster->curr_summ_delivered, m->energy_wh);
        hal_zigbee_notify_attribute_changed(cluster->endpoint,
                                            ZCL_CLUSTER_METERING,
                                            ZCL_ATTR_METERING_CURR_SUMM_DELIVERED);
    }
}
