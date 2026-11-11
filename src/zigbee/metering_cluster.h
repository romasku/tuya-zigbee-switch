#ifndef _METERING_CLUSTER_H_
#define _METERING_CLUSTER_H_

#include <stdint.h>
#include "hal/zigbee.h"
#include "base_components/energy_meter.h"

typedef struct {
    uint8_t              endpoint;
    energy_meter_t *     meter;
    uint64_t             current_summation_delivered;
    uint8_t              status;
    uint8_t              unit_of_measure;
    uint32_t             multiplier;
    uint32_t             divisor;
    uint8_t              summation_formatting;
    uint8_t              metering_device_type;
    uint8_t              reset_trigger; // write any value to reset energy counter
    hal_zigbee_attribute attr_infos[8];
    uint32_t             last_energy_value;
    uint32_t             last_nvm_save_time;
    uint32_t             last_report_time;
    uint64_t             last_reported_energy;
} metering_cluster_t;

void metering_cluster_init(metering_cluster_t *cluster, energy_meter_t *meter);
void metering_cluster_add_to_endpoint(metering_cluster_t *cluster,
                                      hal_zigbee_endpoint *endpoint);
void metering_cluster_update(metering_cluster_t *cluster);
void metering_cluster_report(metering_cluster_t *cluster);
void metering_cluster_load_energy(metering_cluster_t *cluster);
void metering_cluster_save_energy(metering_cluster_t *cluster);
void metering_cluster_reset_energy(metering_cluster_t *cluster);
void metering_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                     uint16_t attribute_id);

#endif /* _METERING_CLUSTER_H_ */
