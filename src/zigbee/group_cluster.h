#ifndef _GROUP_CLUSTER_H_
#define _GROUP_CLUSTER_H_

#include "hal/zigbee.h"

typedef struct {
  hal_zigbee_attribute attr_infos[1];
} zigbee_group_cluster;

void group_cluster_add_to_endpoint(zigbee_group_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint);

#endif
