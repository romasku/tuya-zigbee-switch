#include "group_cluster.h"
#include "cluster_common.h"
#include "consts.h"
#include "hal/zigbee.h"
#include <stdint.h>

const uint8_t groupNameSupport = 0x0;

void group_cluster_add_to_endpoint(zigbee_group_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint) {
    SETUP_ATTR(0, ZCL_ATTR_GROUP_NAME_SUPPORT, ZCL_DATA_TYPE_BITMAP8,
               ATTR_READONLY, groupNameSupport);

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_GROUPS;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 1;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->cluster_count++;
}
