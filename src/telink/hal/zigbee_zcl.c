#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "zcl_multistate_input.h"
#include "zcl_onoff_configuration.h"
#pragma pack(pop)

#include "telink_size_t_hack.h"

#include "hal/zigbee.h"
#include "telink_zigbee_hal.h"
#include "zigbee/consts.h"
#ifdef BATTERY_POWERED
#include "zigbee/battery_cluster.h"
#endif

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

#ifdef ZB_ED_ROLE
static void telink_zcl_data_confirm_cb(void *arg) {
    // arg points to apsdeDataConf_t — we only care about the event itself.
    (void)arg;
    telink_zigbee_hal_end_active_period();
}
#endif

#ifdef BATTERY_POWERED
extern status_t zcl_powerCfg_register(u8 endpoint, u16 manuCode, u8 attrNum, const zclAttrInfo_t *attrTbl, cluster_forAppCb_t cb);
#endif

static cluster_registerFunc_t get_register_func_by_cluster_id(u16 cluster_id) {
    if (cluster_id == ZCL_CLUSTER_GEN_BASIC) {
        return zcl_basic_register;
    }
#ifdef BATTERY_POWERED
    if (cluster_id == ZCL_CLUSTER_GEN_POWER_CFG ||
        cluster_id == ZCL_CLUSTER_POWER_CFG) {
        return zcl_powerCfg_register;
    }
#endif
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
        return zcl_windowCovering_register;
    }
    return NULL;
}

static status_t cmd_callback(u8 endpoint, u16 clusterId, u8 cmdId,
                             void *cmdPayload) {
    hal_zigbee_cluster *cluster = hal_zigbee_find_cluster(
        hal_endpoints, hal_endpoints_cnt, endpoint, clusterId);

    if (cluster && cluster->cmd_callback) {
        return cluster->cmd_callback(endpoint, clusterId, cmdId, cmdPayload);
    }
    return(ZCL_STA_SUCCESS);
}

static status_t cmd_callback_on_off(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId,
                                    void *cmdPayload) {
    return cmd_callback(pAddrInfo->dstEp, ZCL_CLUSTER_GEN_ON_OFF, cmdId,
                        cmdPayload);
}

static status_t cmd_callback_window_covering(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId,
                                             void *cmdPayload) {
    return cmd_callback(pAddrInfo->dstEp, ZCL_CLUSTER_CLOSURES_WINDOW_COVERING, cmdId,
                        cmdPayload);
}

static cluster_forAppCb_t get_cmd_callback_by_cluster_id(u16 cluster_id) {
    if (cluster_id == ZCL_CLUSTER_GEN_ON_OFF) {
        return cmd_callback_on_off;
    }
    if (cluster_id == ZCL_CLUSTER_CLOSURES_WINDOW_COVERING) {
        return cmd_callback_window_covering;
    }
    return NULL;
}

static void zcl_incoming_message_callback(zclIncoming_t *pInHdlrMsg) {
#ifdef BATTERY_POWERED
    // Refresh battery value before responding to a read request
    if (pInHdlrMsg->hdr.cmd == ZCL_CMD_READ &&
        (pInHdlrMsg->msg->indInfo.cluster_id == ZCL_CLUSTER_GEN_POWER_CFG ||
         pInHdlrMsg->msg->indInfo.cluster_id == ZCL_CLUSTER_POWER_CFG)) {
        battery_cluster_update_on_event();
    }
#endif
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
        af_endpointRegister(endpoint->endpoint, endpoint_desc_ptr, zcl_rx_handler,
#ifdef ZB_ED_ROLE
                            telink_zcl_data_confirm_cb
#else
                            NULL
#endif
                            );

        u8 cluster_count = cluster_info_ptr - endpoint_first_cluster_ptr;
        zcl_register(endpoint->endpoint, cluster_count, endpoint_first_cluster_ptr);

        endpoint_desc_ptr++;
    }
}

hal_zigbee_status_t hal_zigbee_set_attribute_value(uint8_t endpoint,
                                                    uint16_t cluster_id,
                                                    uint16_t attribute_id,
                                                    uint8_t *value) {
    status_t status = zcl_setAttrVal(endpoint, cluster_id, attribute_id, value);
    if (status != ZCL_STA_SUCCESS) {
        return HAL_ZIGBEE_ERROR;
    }
    return HAL_ZIGBEE_OK;
}

void hal_zigbee_notify_attribute_changed(uint8_t endpoint, uint16_t cluster_id,
                                         uint16_t attribute_id) {
    report_handler();
    telink_zigbee_hal_request_active_period();
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
        zcl_sendReportCmd(
            endpoint, &dstEpInfo, TRUE, ZCL_FRAME_SERVER_CLIENT_DIR, cluster_id,
            pAttrEntry->id, pAttrEntry->type, pAttrEntry->data);
#ifdef ZB_ED_ROLE
        // Ensure the device stays awake until the frame is confirmed.
        telink_zigbee_hal_request_active_period();
#endif
    }
    return HAL_ZIGBEE_OK;
}

void hal_zigbee_register_on_attribute_change_callback(
    hal_attribute_change_callback_t callback) {
    attribute_change_callback = callback;
}

// Internal interface functions

af_simple_descriptor_t *telink_zigbee_hal_zcl_get_descriptors(void) {
    return endpoint_descriptors;
}
