#ifndef _METERING_CLUSTER_H_
#define _METERING_CLUSTER_H_

#include <stdint.h>

#include "base_components/metering_bl0937.h"
#include "hal/zigbee.h"

/*
 * Exposes BL0937 measurements through two standard ZCL server clusters:
 *   - Electrical Measurement (0x0B04): rmsVoltage / rmsCurrent / activePower
 *     with the AC formatting divisors set so values decode as
 *     V = raw/10, A = raw/1000, W = raw/10.
 *   - Metering (0x0702): currentSummationDelivered as uint48 in Wh with
 *     divisor 1000 (=> kWh in Z2M).
 *
 * Attribute values are refreshed and change-notified from the driver's
 * on_update callback; the coordinator's configured ZCL reporting then
 * takes care of pushing them out.
 */

typedef struct {
    uint8_t              endpoint;
    metering_bl0937_t *  metering;

    // Electrical Measurement cluster storage
    uint32_t             measurement_type;    // bitmap32, bit0 = AC active
    uint16_t             ac_voltage_mult, ac_voltage_div;
    uint16_t             ac_current_mult, ac_current_div;
    uint16_t             ac_power_mult, ac_power_div;
    hal_zigbee_attribute em_attr_infos[10];

    // Metering cluster storage
    uint8_t              curr_summ_delivered[6]; // uint48, little endian, Wh
    uint8_t              status;                 // bitmap8, 0 = no error
    uint8_t              unit_of_measure;        // 0 = kWh
    uint32_t             multiplier;             // uint24 (stored low 3 bytes)
    uint32_t             divisor;                // uint24 (stored low 3 bytes)
    uint8_t              summation_formatting;
    uint8_t              metering_device_type;   // 0 = electric metering
    hal_zigbee_attribute mt_attr_infos[7];

    // Last reported values (for change-only notifications)
    uint16_t             last_voltage_dv;
    uint16_t             last_current_ma;
    int16_t              last_power_dw;
    uint32_t             last_energy_wh;
} zigbee_metering_cluster;

/**
 * Register both measurement clusters on the given endpoint and hook the
 * driver's update callback. Adds 2 entries to endpoint->clusters.
 */
void metering_cluster_add_to_endpoint(zigbee_metering_cluster *cluster,
                                      hal_zigbee_endpoint *endpoint);

#endif
