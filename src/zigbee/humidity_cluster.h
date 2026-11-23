#ifndef _HUMIDITY_CLUSTER_H_
#define _HUMIDITY_CLUSTER_H_

#include "base_components/onewire.h"
#include "hal/zigbee.h"
#include <stdint.h>

typedef struct {
    uint16_t             measured_value;
    uint8_t              endpoint;
    hal_zigbee_attribute attr_infos[4];
} zigbee_humidity_cluster;

extern zigbee_humidity_cluster humidity_clusters[OW_MAX_SENSORS];
extern uint8_t humidity_clusters_cnt;

void humidity_cluster_add_to_endpoint(zigbee_humidity_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint);
/* force reports every endpoint, including one whose value did not change. */
void humidity_cluster_publish(const uint16_t rh_percent_x100[OW_MAX_SENSORS],
                              uint8_t force);

#endif
