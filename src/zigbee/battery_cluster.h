#ifndef _BATTERY_CLUSTER_H_
#define _BATTERY_CLUSTER_H_

#include "hal/zigbee.h"

#ifdef BATTERY_POWERED

typedef struct {
    uint8_t percentage_remaining;       // ZCL 0-200 (0.5% steps)
    uint8_t voltage_100mv;              // ZCL 0-255 (100mV steps)
    uint8_t endpoint;
    hal_zigbee_attribute attr_infos[3]; // voltage + percentage + cluster_revision
} zigbee_battery_cluster;

void battery_cluster_add_to_endpoint(zigbee_battery_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint);
void battery_cluster_update(zigbee_battery_cluster *cluster);
void battery_cluster_update_on_event(void);
void battery_cluster_force_report(void);

#endif // BATTERY_POWERED

#endif // _BATTERY_CLUSTER_H_
