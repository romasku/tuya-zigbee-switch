#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_cover_switch_config.h"
#include "zcl_include.h"
#include "zcl_multistate_input.h"
#include "zcl_window_covering_custom.h"
#include "zcl_onoff_configuration.h"
#pragma pack(pop)

#include "telink_size_t_hack.h"

#include "hal/zigbee.h"
#include "telink_zigbee_hal.h"
#include "zigbee/battery_cluster.h"
#include "zigbee/consts.h"

// Storage for Telink endpoint configuration
static af_simple_descriptor_t endpoint_descriptors[MAX_ENDPOINTS];
static u16 in_clusters[MAX_IN_CLUSTERS];
static u16 out_clusters[MAX_OUT_CLUSTERS];
static zcl_specClusterInfo_t cluster_infos[MAX_IN_CLUSTERS + MAX_OUT_CLUSTERS];
static zclAttrInfo_t         attr_tables[MAX_ATTRS];

// HAL state tracking
static hal_zigbee_endpoint *hal_endpoints = NULL;
static uint8_t hal_endpoints_cnt          = 0;
static hal_attribute_change_callback_t attribute_change_callback = NULL;
static hal_zcl_activity_callback_t     zcl_activity_callback     = NULL;

static cluster_registerFunc_t get_register_func_by_cluster_id(u16 cluster_id) {
    if (cluster_id == ZCL_CLUSTER_GEN_BASIC) {
        return zcl_basic_register;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_POWER_CFG ||
        cluster_id == ZCL_CLUSTER_POWER_CFG) {
        return zcl_powerCfg_register;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_IDENTIFY) {
        return zcl_identify_register;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_GROUPS) {
        return zcl_group_register;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_ON_OFF_SWITCH_CONFIG) {
        return zcl_onoff_configuration_register;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_LEVEL_CONTROL) {
        return zcl_level_register;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_ON_OFF) {
        return zcl_onOff_register;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_MULTISTATE_INPUT_BASIC) {
        return zcl_multistate_input_register;
    }
    if (cluster_id == ZCL_CLUSTER_CLOSURES_WINDOW_COVERING) {
        return zcl_windowCovering_custom_register;
    }
    if (cluster_id == ZCL_CLUSTER_COVER_SWITCH_CONFIG) {
        return zcl_cover_switch_config_register;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_POLL_CONTROL) {
        return zcl_pollCtrl_register;
    }
    return NULL;
}

static status_t cmd_callback(u8 endpoint, u16 clusterId, u8 cmdId,
                             void *cmdPayload, u16 cmdPayloadLen) {
    hal_zigbee_cluster *cluster = hal_zigbee_find_cluster(
        hal_endpoints, hal_endpoints_cnt, endpoint, clusterId);

    if (cluster && cluster->cmd_callback) {
        hal_zigbee_cmd_result_t status = cluster->cmd_callback(
            endpoint, clusterId, cmdId, cmdPayload, cmdPayloadLen);
        if (status == HAL_ZIGBEE_CMD_PROCESSED) {
            return ZCL_STA_SUCCESS;
        } else if (status == HAL_ZIGBEE_INVALID_VALUE) {
            return ZCL_STA_INVALID_VALUE;
        } else if (status == HAL_ZIGBEE_MALFORMED_COMMAND) {
            return ZCL_STA_MALFORMED_COMMAND;
        } else if (status == HAL_ZIGBEE_ACTION_DENIED) {
            return ZCL_STA_ACTION_DENIED;
        } else if (status == HAL_ZIGBEE_CMD_SKIPPED) {
            return ZCL_STA_UNSUP_CLUSTER_COMMAND;
        }
    }
    return(ZCL_STA_SUCCESS);
}

static zclIncoming_t *cmd_incoming_from_addr_info(zclIncomingAddrInfo_t *pAddrInfo) {
    // Telink passes &zclIncoming_t.addrInfo into clusterAppCb, so recover the
    // enclosing message to access the raw payload bytes and length.
    return (zclIncoming_t *)((char *)pAddrInfo - offsetof(zclIncoming_t, addrInfo));
}

static status_t cmd_callback_on_off(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId,
                                    void *cmdPayload) {
    zclIncoming_t *pInMsg = cmd_incoming_from_addr_info(pAddrInfo);

    return cmd_callback(pAddrInfo->dstEp, ZCL_CLUSTER_GEN_ON_OFF, cmdId,
                        pInMsg->pData, pInMsg->dataLen);
}

static status_t cmd_callback_window_covering(zclIncomingAddrInfo_t *pAddrInfo,
                                             u8 cmdId, void *cmdPayload) {
    // cmd_incoming_from_addr_info didn't work here for some reason and I had to use
    // the cmdPayload directly. Not sure why, I don't have serial logs to investigate.
    return cmd_callback(pAddrInfo->dstEp, ZCL_CLUSTER_CLOSURES_WINDOW_COVERING,
                        cmdId, cmdPayload, 1);
}

static status_t cmd_callback_level_control(zclIncomingAddrInfo_t *pAddrInfo,
                                           u8 cmdId, void *cmdPayload) {
    zclIncoming_t *pInMsg = cmd_incoming_from_addr_info(pAddrInfo);

    return cmd_callback(pAddrInfo->dstEp, ZCL_CLUSTER_GEN_LEVEL_CONTROL, cmdId,
                        pInMsg->pData, pInMsg->dataLen);
}

static status_t cmd_callback_poll_control(zclIncomingAddrInfo_t *pAddrInfo,
                                          u8 cmdId, void *cmdPayload) {
    zclIncoming_t *pInMsg = cmd_incoming_from_addr_info(pAddrInfo);

    return cmd_callback(pAddrInfo->dstEp, ZCL_CLUSTER_GEN_POLL_CONTROL, cmdId,
                        pInMsg->pData, pInMsg->dataLen);
}

static cluster_forAppCb_t get_cmd_callback_by_cluster_id(u16 cluster_id) {
    if (cluster_id == ZCL_CLUSTER_GEN_LEVEL_CONTROL) { // Level Control cluster
        return cmd_callback_level_control;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_ON_OFF) { // On/Off cluster
        return cmd_callback_on_off;
    }
    if (cluster_id == ZCL_CLUSTER_CLOSURES_WINDOW_COVERING) {
        return cmd_callback_window_covering;
    }
    if (cluster_id == ZCL_CLUSTER_GEN_POLL_CONTROL) {
        return cmd_callback_poll_control;
    }
    return NULL;
}

static void zcl_incoming_message_callback(zclIncoming_t *pInHdlrMsg) {
    if (pInHdlrMsg->hdr.cmd == ZCL_CMD_WRITE ||
        pInHdlrMsg->hdr.cmd == ZCL_CMD_WRITE_NO_RSP) {
        if (attribute_change_callback == NULL) {
            return;
        }
        zclWriteCmd_t *writeCmd = (zclWriteCmd_t *)pInHdlrMsg->attrCmd;
        for (u8 i = 0; i < writeCmd->numAttr; i++) {
            attribute_change_callback(pInHdlrMsg->msg->indInfo.dst_ep,
                                      pInHdlrMsg->msg->indInfo.cluster_id,
                                      writeCmd->attrList[i].attrID);
        }
    }
}

static void af_rx_callback(void *arg) {
    if (zcl_activity_callback != NULL) {
        zcl_activity_callback();
    }
    zcl_rx_handler(arg);
}

void telink_zigbee_hal_zcl_init(hal_zigbee_endpoint *endpoints,
                                uint8_t endpoints_cnt) {
    zcl_init(zcl_incoming_message_callback);
    zcl_reportingTabInit();

    hal_endpoints     = endpoints;
    hal_endpoints_cnt =
        endpoints_cnt < MAX_ENDPOINTS ? endpoints_cnt : MAX_ENDPOINTS;
    af_simple_descriptor_t *endpoint_desc_ptr = endpoint_descriptors;
    u16 *in_cluster_ptr  = in_clusters;
    u16 *out_cluster_ptr = out_clusters;
    zcl_specClusterInfo_t *cluster_info_ptr = cluster_infos;
    zclAttrInfo_t *        attr_table_ptr   = attr_tables;

    for (hal_zigbee_endpoint *endpoint = endpoints;
         endpoint < endpoints + hal_endpoints_cnt; endpoint++) {
        endpoint_desc_ptr->endpoint            = endpoint->endpoint;
        endpoint_desc_ptr->app_profile_id      = endpoint->profile_id;
        endpoint_desc_ptr->app_dev_id          = endpoint->device_id;
        endpoint_desc_ptr->app_dev_ver         = endpoint->device_version;
        endpoint_desc_ptr->app_in_cluster_lst  = in_cluster_ptr;
        endpoint_desc_ptr->app_out_cluster_lst = out_cluster_ptr;

        zcl_specClusterInfo_t *endpoint_first_cluster_ptr = cluster_info_ptr;
        for (hal_zigbee_cluster *cluster = endpoint->clusters;
             cluster < endpoint->clusters + endpoint->cluster_count; cluster++) {
            if (cluster->is_server) {
                *in_cluster_ptr = cluster->cluster_id;
                in_cluster_ptr++;
                endpoint_desc_ptr->app_in_cluster_count++;
            } else {
                *out_cluster_ptr = cluster->cluster_id;
                out_cluster_ptr++;
                endpoint_desc_ptr->app_out_cluster_count++;
            }
            if (cluster->cluster_id == ZCL_CLUSTER_OTA) {
                continue;
            }
            cluster_info_ptr->clusterId           = cluster->cluster_id;
            cluster_info_ptr->manuCode            = 0;
            cluster_info_ptr->attrTbl             = attr_table_ptr;
            cluster_info_ptr->attrNum             = 0;
            cluster_info_ptr->clusterRegisterFunc =
                get_register_func_by_cluster_id(cluster->cluster_id);
            cluster_info_ptr->clusterAppCb =
                get_cmd_callback_by_cluster_id(cluster->cluster_id);
            for (hal_zigbee_attribute *attr = cluster->attributes;
                 attr < cluster->attributes + cluster->attribute_count; attr++) {
                attr_table_ptr->id     = attr->attribute_id;
                attr_table_ptr->type   = attr->data_type_id;
                attr_table_ptr->access =
                    ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE;
                if (attr->flag == ATTR_WRITABLE) {
                    attr_table_ptr->access |= ACCESS_CONTROL_WRITE;
                }
                attr_table_ptr->data = attr->value;
                attr_table_ptr++;
                cluster_info_ptr->attrNum++;
            }

            cluster_info_ptr++;
        }
        af_endpointRegister(endpoint->endpoint, endpoint_desc_ptr,
                            af_rx_callback, NULL);
        u8 cluster_count = cluster_info_ptr - endpoint_first_cluster_ptr;
        zcl_register(endpoint->endpoint, cluster_count, endpoint_first_cluster_ptr);

        endpoint_desc_ptr++;
    }
}

void hal_zigbee_notify_attribute_changed(uint8_t endpoint, uint16_t cluster_id,
                                         uint16_t attribute_id) {
    report_handler(); // Trigger reporting if needed
}

hal_zigbee_status_t hal_zigbee_send_cmd_to_bindings(const hal_zigbee_cmd *cmd) {
    epInfo_t dstEpInfo;

    TL_SETSTRUCTCONTENT(dstEpInfo, 0);

    dstEpInfo.profileId   = HA_PROFILE_ID;
    dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
    zcl_sendCmd(cmd->endpoint, &dstEpInfo, cmd->cluster_id, cmd->command_id,
                cmd->cluster_specific,
                cmd->direction == HAL_ZIGBEE_DIR_CLIENT_TO_SERVER
                  ? ZCL_FRAME_CLIENT_SERVER_DIR
                  : ZCL_FRAME_SERVER_CLIENT_DIR,
                cmd->disable_default_rsp, cmd->manufacturer_code, ZCL_SEQ_NUM,
                cmd->payload_len, (u8 *)cmd->payload);

    return HAL_ZIGBEE_OK;
}

hal_zigbee_status_t
hal_zigbee_send_report_attr(uint8_t endpoint, uint16_t cluster_id,
                            uint16_t attr_id, uint8_t zcl_type_id,
                            const void *value, uint8_t value_len) {
    if (zb_isDeviceJoinedNwk()) {
        epInfo_t dstEpInfo;
        TL_SETSTRUCTCONTENT(dstEpInfo, 0);

        dstEpInfo.profileId   = HA_PROFILE_ID;
        dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;

        zclAttrInfo_t *pAttrEntry;
        pAttrEntry = zcl_findAttribute(endpoint, cluster_id, attr_id);
        zcl_sendReportCmd(endpoint, &dstEpInfo, TRUE, ZCL_FRAME_SERVER_CLIENT_DIR,
                          cluster_id, pAttrEntry->id, pAttrEntry->type,
                          pAttrEntry->data);
    }
    return HAL_ZIGBEE_OK;
}

void hal_zigbee_register_on_attribute_change_callback(
    hal_attribute_change_callback_t callback) {
    attribute_change_callback = callback;
}

void hal_zigbee_register_on_zcl_activity_callback(hal_zcl_activity_callback_t callback) {
    zcl_activity_callback = callback;
}

// Internal interface functions

af_simple_descriptor_t *telink_zigbee_hal_zcl_get_descriptors(void) {
    return endpoint_descriptors;
}
