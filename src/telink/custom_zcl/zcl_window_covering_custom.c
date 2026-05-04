#pragma pack(push, 1)
#include "zcl_include.h"
#pragma pack(pop)

/*
 * Custom Window Covering Cluster Implementation
 *
 * This custom implementation is required because the SDK's default
 * zcl_windowCovering_register() only handles UP_OPEN, DOWN_CLOSE, and STOP
 * commands. When a GO_TO_LIFT_PERCENTAGE command is received, the SDK returns
 * ZCL_STA_UNSUP_CLUSTER_COMMAND even though the command is defined in the spec.
 *
 * This custom handler adds support for GO_TO_LIFT_PERCENTAGE by properly parsing
 * the single-byte percentage payload and passing it to the application callback.
 */

static status_t zcl_windowCovering_custom_cmdHandler(zclIncoming_t *incoming);

_CODE_ZCL_ status_t zcl_windowCovering_custom_register(u8 endpoint, u16 manuCode, u8 attrNum,
                                                       const zclAttrInfo_t attrTbl[],
                                                       cluster_forAppCb_t cb) {
    return(zcl_registerCluster(endpoint, ZCL_CLUSTER_CLOSURES_WINDOW_COVERING, manuCode, attrNum,
                               attrTbl, zcl_windowCovering_custom_cmdHandler, cb));
}

_CODE_ZCL_ static status_t zcl_windowCovering_custom_cmdHandler(zclIncoming_t *incoming) {
    if (incoming->hdr.frmCtrl.bf.dir != ZCL_FRAME_CLIENT_SERVER_DIR) {
        return(ZCL_STA_UNSUP_CLUSTER_COMMAND);
    }

    void *payload = NULL;
    switch (incoming->hdr.cmd) {
    case ZCL_CMD_UP_OPEN:
    case ZCL_CMD_DOWN_CLOSE:
    case ZCL_CMD_STOP:
        break;
    case ZCL_CMD_GO_TO_LIFT_PERCENTAGE:
        payload = (void *)incoming->pData;
        break;
    default:
        return(ZCL_STA_UNSUP_CLUSTER_COMMAND);
    }

    if (!incoming->clusterAppCb) {
        return(ZCL_STA_FAILURE);
    }

    return(incoming->clusterAppCb(&(incoming->addrInfo), incoming->hdr.cmd, payload));
}
