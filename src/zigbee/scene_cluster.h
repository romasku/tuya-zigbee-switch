#ifndef _SCENE_CLUSTER_H_
#define _SCENE_CLUSTER_H_

#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"
#include "endpoint.h"

#define SCENE_CLUSTER_ATTR_NUM 7

typedef struct zigbee_scene_cluster
{
    u8 scene_count;
    u8 current_scene;
    u16 current_group;
    bool scene_valid;
    addrExt_t last_configured_by;

    struct zigbee_relay_cluster *relay_cluster;

    zclAttrInfo_t attr_infos[SCENE_CLUSTER_ATTR_NUM];
} zigbee_scene_cluster;

void scene_cluster_add_to_endpoint(zigbee_scene_cluster *cluster, zigbee_endpoint *endpoint);

status_t scene_cluster_callback(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);

#endif