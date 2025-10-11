#include "scene_cluster.h"
#include "relay_cluster.h"
#include "cluster_common.h"

zigbee_scene_cluster *scene_cluster_by_endpoint[10] = {};

static void store_scene(zcl_sceneEntry_t *scene_entry, zigbee_scene_cluster *cluster);
static void recall_scene(zcl_sceneEntry_t *scene_entry, zigbee_scene_cluster *cluster);

static const u8 scene_name_support = 0x01;

void scene_cluster_add_to_endpoint(zigbee_scene_cluster *cluster, zigbee_endpoint *endpoint)
{
    scene_cluster_by_endpoint[endpoint->index] = cluster;

    SETUP_ATTR(0, ZCL_ATTRID_SCENE_SCENE_COUNT, ZCL_DATA_TYPE_UINT8, ACCESS_CONTROL_READ, cluster->scene_count);
    SETUP_ATTR(1, ZCL_ATTRID_SCENE_CURRENT_SCENE, ZCL_DATA_TYPE_UINT8, ACCESS_CONTROL_READ, cluster->current_scene);
    SETUP_ATTR(2, ZCL_ATTRID_SCENE_CURRENT_GROUP, ZCL_DATA_TYPE_UINT16, ACCESS_CONTROL_READ, cluster->current_group);
    SETUP_ATTR(3, ZCL_ATTRID_SCENE_SCENE_VALID, ZCL_DATA_TYPE_BOOLEAN, ACCESS_CONTROL_READ, cluster->scene_valid);
    SETUP_ATTR(4, ZCL_ATTRID_SCENE_NAME_SUPPORT, ZCL_DATA_TYPE_BITMAP8, ACCESS_CONTROL_READ, scene_name_support);
    SETUP_ATTR(5, ZCL_ATTRID_SCENE_LAST_CONFIG_BY, ZCL_DATA_TYPE_IEEE_ADDR, ACCESS_CONTROL_READ, cluster->last_configured_by);
    SETUP_ATTR(6, ZCL_ATTRID_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16, ACCESS_CONTROL_READ, zcl_attr_global_clusterRevision);

    zigbee_endpoint_add_cluster(endpoint, 1, ZCL_CLUSTER_GEN_SCENES);
    zcl_specClusterInfo_t *info = zigbee_endpoint_reserve_info(endpoint);
    info->clusterId = ZCL_CLUSTER_GEN_SCENES;
    info->manuCode = MANUFACTURER_CODE_NONE;
    info->attrNum = SCENE_CLUSTER_ATTR_NUM;
    info->attrTbl = cluster->attr_infos;
    info->clusterRegisterFunc = zcl_scene_register;
    info->clusterAppCb = scene_cluster_callback;
}

status_t scene_cluster_callback(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
    zigbee_scene_cluster *cluster = scene_cluster_by_endpoint[pAddrInfo->dstEp];

    status_t status = ZCL_STA_SUCCESS;

    if (pAddrInfo->dirCluster == ZCL_FRAME_CLIENT_SERVER_DIR)
    {
        switch (cmdId)
        {
        case ZCL_CMD_SCENE_STORE_SCENE:
            store_scene(cmdPayload, cluster);
            break;
        case ZCL_CMD_SCENE_RECALL_SCENE:
            recall_scene(cmdPayload, cluster);
            break;
        default:
            break;
        }
    }
    else
    {
        status = ZCL_STA_UNSUP_CLUSTER_COMMAND;
    }

    return status;
}

static void recall_scene(zcl_sceneEntry_t *scene_entry, zigbee_scene_cluster *cluster)
{
    u8 *pData = scene_entry->extField;
    u16 clusterID = 0xFFFF;
    u8 extLen = 0;

    while (pData < scene_entry->extField + scene_entry->extFieldLen) {
        clusterID = BUILD_U16(pData[0], pData[1]);
        pData += 2;//cluster id

        extLen = *pData++;//length

        if (clusterID == ZCL_CLUSTER_GEN_ON_OFF && cluster->relay_cluster) {
            if (extLen >= 1) {
                u8 onOff = *pData++;

                relay_cluster_set_on_off(cluster->relay_cluster, onOff, true);
                extLen--;
            }
        }

        pData += extLen;
    }
}

static void store_scene(zcl_sceneEntry_t *scene_entry, zigbee_scene_cluster *cluster)
{
    u8 extLen = 0;

    if (cluster->relay_cluster) {
        bool on_off = cluster->relay_cluster->on_off;

        scene_entry->extField[extLen++] = LO_UINT16(ZCL_CLUSTER_GEN_ON_OFF);
        scene_entry->extField[extLen++] = HI_UINT16(ZCL_CLUSTER_GEN_ON_OFF);
        scene_entry->extField[extLen++] = sizeof(u8);
        scene_entry->extField[extLen++] = on_off;
    }

    // Add other clusters to save here...

    scene_entry->extFieldLen = extLen;
}