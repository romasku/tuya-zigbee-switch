#ifndef _TEMPERATURE_CLUSTER_H_
#define _TEMPERATURE_CLUSTER_H_

#include "base_components/onewire.h"
#include "hal/tasks.h"
#include "hal/zigbee.h"

typedef struct {
    int16_t              measured_value;
    uint8_t              endpoint;
    /* ZCL char string: length byte, then up to 23 id characters. */
    uint8_t              sensor_id[24];
    hal_zigbee_attribute attr_infos[5];
} zigbee_temperature_cluster;

extern zigbee_temperature_cluster temperature_clusters[OW_MAX_SENSORS];
extern uint8_t temperature_clusters_cnt;

void temperature_cluster_add_to_endpoint(zigbee_temperature_cluster *cluster,
                                        hal_zigbee_endpoint *endpoint);
/* Search the bus once. Call after NV reads and hal_zigbee_init. */
void dallas1wire_clusters_start(void);

#endif
