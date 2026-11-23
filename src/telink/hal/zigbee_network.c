#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#pragma pack(pop)

#include "ota.h"

#include "telink_size_t_hack.h"

#include "hal/zigbee.h"
#include "telink_zigbee_hal.h"
#include "version_cfg.h"

// Forward definitions
void bdb_init_callback(u8 status, u8 joinedNetwork);
void bdb_commissioning_callback(u8 status, void *arg);
void bdb_identify_callback(u8 endpoint, u16 srcAddr, u16 identifyTime);

void zdo_leave_indication_callback(nlme_leave_ind_t *pLeaveInd);
void zdo_leave_confirmation_callback(nlme_leave_cnf_t *pLeaveCnf);

// Network status tracking
static hal_network_status_change_callback_t network_status_change_callback =
    NULL;
static bool steeringInProgress = 0;

// Telink ZDO callbacks
zdo_appIndCb_t zdo_callbacks = {
    bdb_zdoStartDevCnf,              // start device cnf cb
    NULL,                            // reset cnf cb
    NULL,                            // device announce indication cb
    zdo_leave_indication_callback,   // leave ind cb
    zdo_leave_confirmation_callback, // leave cnf cb
    NULL,                            // nwk update ind cb
    NULL,                            // permit join ind cb
    NULL,                            // nlme sync cnf cb
    NULL,                            // tc join ind cb
    NULL,                            // tc detects that the frame counter is near limit
};

// Telink BDB storage
static bdb_commissionSetting_t bdb_commission_setting = {
    .linkKey.tcLinkKey.keyType = SS_GLOBAL_LINK_KEY,
    .linkKey.tcLinkKey.key     = (u8 *)tcLinkKeyCentralDefault,

    .linkKey.distributeLinkKey.keyType = MASTER_KEY,
    .linkKey.distributeLinkKey.key     = (u8 *)linkKeyDistributedMaster,

    .linkKey.touchLinkKey.keyType = MASTER_KEY,
    .linkKey.touchLinkKey.key     = (u8 *)touchLinkKeyMaster,

    .touchlinkEnable       =                              0,
    .touchlinkChannel      = DEFAULT_CHANNEL,
    .touchlinkLqiThreshold =                           0xA0,
};

static bdb_appCb_t device_bdb_cb = {
    bdb_init_callback,
    bdb_commissioning_callback,
    bdb_identify_callback,
    NULL,
};

static void notify_about_network_status_change() {
    if (network_status_change_callback != NULL) {
        network_status_change_callback(hal_zigbee_get_network_status());
    }
}

void zdo_leave_indication_callback(nlme_leave_ind_t *pLeaveInd) {
}

void zdo_leave_confirmation_callback(nlme_leave_cnf_t *pLeaveCnf) {
    notify_about_network_status_change();
}

void bdb_init_callback(u8 status, u8 joinedNetwork) {
    if (status == BDB_INIT_STATUS_SUCCESS) {
        if (joinedNetwork) {
            ota_queryStart(OTA_QUERY_INTERVAL);
      #ifdef ZB_ED_ROLE
            zb_setPollRate(POLL_RATE);
      #endif
        }
    } else {
        if (joinedNetwork) {
            zb_rejoinReqWithBackOff(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
        }
    }
    notify_about_network_status_change();
}

#if UART_PRINTF_MODE
static const char *bdb_commission_status_text(u8 status) {
    switch (status) {
    case BDB_COMMISSION_STA_SUCCESS:
        return "success";
    case BDB_COMMISSION_STA_IN_PROGRESS:
        return "in progress";
    case BDB_COMMISSION_STA_NOT_AA_CAPABLE:
        return "not address-assignment capable";
    case BDB_COMMISSION_STA_NO_NETWORK:
        return "no network to associate with";
    case BDB_COMMISSION_STA_TARGET_FAILURE:
        return "target failure";
    case BDB_COMMISSION_STA_FORMATION_FAILURE:
        return "formation failure";
    case BDB_COMMISSION_STA_NO_IDENTIFY_QUERY_RESPONSE:
        return "no identify query response";
    case BDB_COMMISSION_STA_BINDING_TABLE_FULL:
        return "binding table full";
    case BDB_COMMISSION_STA_NO_SCAN_RESPONSE:
        return "no scan response";
    case BDB_COMMISSION_STA_NOT_PERMITTED:
        return "not permitted";
    case BDB_COMMISSION_STA_TCLK_EX_FAILURE:
        return "trust center link key exchange failed";
    case BDB_COMMISSION_STA_PARENT_LOST:
        return "parent lost";
    case BDB_COMMISSION_STA_REJOIN_FAILURE:
        return "rejoin failed";
    case BDB_COMMISSION_STA_FORMATION_DONE:
        return "formation done";
    default:
        return "unknown";
    }
}
#endif

void bdb_commissioning_callback(u8 status, void *arg) {
#if UART_PRINTF_MODE
    printf("BDB commissioning callback, status: %d (%s)\r\n", status,
           bdb_commission_status_text(status));
#else
    printf("BDB commissioning callback, status: %d\r\n", status);
#endif
    switch (status) {
    case BDB_COMMISSION_STA_SUCCESS:
        ota_queryStart(OTA_QUERY_INTERVAL);
#ifdef ZB_ED_ROLE
        // Need set poll rate manually,
        // to avoid bugs related to no poll task
        // after fast re-connect.
        zb_setPollRate(POLL_RATE);
        printf("Set poll rate to %d\r\n", POLL_RATE);
#endif
        steeringInProgress = 0;
        break;
    case BDB_COMMISSION_STA_IN_PROGRESS:
        break;
    case BDB_COMMISSION_STA_NOT_AA_CAPABLE:
        break;
    case BDB_COMMISSION_STA_NO_NETWORK:
    case BDB_COMMISSION_STA_TCLK_EX_FAILURE:
    case BDB_COMMISSION_STA_TARGET_FAILURE: {
        steeringInProgress = 0;
    } break;
    case BDB_COMMISSION_STA_FORMATION_FAILURE:
        break;
    case BDB_COMMISSION_STA_NO_IDENTIFY_QUERY_RESPONSE:
        break;
    case BDB_COMMISSION_STA_BINDING_TABLE_FULL:
        break;
    case BDB_COMMISSION_STA_NOT_PERMITTED:
        break;
    case BDB_COMMISSION_STA_NO_SCAN_RESPONSE:
    case BDB_COMMISSION_STA_PARENT_LOST:
        zb_rejoinReqWithBackOff(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
        break;
    case BDB_COMMISSION_STA_REJOIN_FAILURE:
        if (!zb_isDeviceFactoryNew()) {
            zb_rejoinReqWithBackOff(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
        }
        break;
    default:
        break;
    }
    notify_about_network_status_change();
}

void bdb_identify_callback(u8 endpoint, u16 srcAddr, u16 identifyTime) {
    // Not implemented
}

hal_zigbee_network_status_t hal_zigbee_get_network_status(void) {
    if (zb_isDeviceJoinedNwk()) {
        return HAL_ZIGBEE_NETWORK_JOINED;
    }
    if (steeringInProgress) {
        return HAL_ZIGBEE_NETWORK_JOINING;
    }
    return HAL_ZIGBEE_NETWORK_NOT_JOINED;
}

void hal_register_on_network_status_change_callback(
    hal_network_status_change_callback_t callback) {
    network_status_change_callback = callback;
    notify_about_network_status_change();
}

void hal_zigbee_leave_network(void) {
    nlme_leave_req_t leaveReq;

    TL_SETSTRUCTCONTENT(leaveReq, 0);
    leaveReq.removeChildren = 1;
    leaveReq.rejoin         = 0;
    zb_nlmeLeaveReq(&leaveReq);
    notify_about_network_status_change();
}

void hal_zigbee_start_network_steering(void) {
    printf("Starting network steering\r\n");
    u8 res = bdb_networkSteerStart();
    if (res == 0) {
        steeringInProgress = 1;
    } else {
        printf("Failed to start network steering, status: %d\r\n", res);
    }
}

hal_zigbee_status_t hal_zigbee_send_announce(void) {
    if (zb_zdoSendDevAnnance() != RET_OK) {
        return HAL_ZIGBEE_ERR_SEND_FAILED;
    }
    return HAL_ZIGBEE_OK;
}

void hal_zigbee_set_poll_rate_ms(uint32_t poll_rate_ms) {
    zb_setPollRate(poll_rate_ms);
}

uint32_t hal_zigbee_get_poll_rate_ms(void) {
    return zb_getPollRate();
}

// Internal interface functions

#if UART_PRINTF_MODE
/* Called from bdb.c in debug builds, after network discovery and before
 * zb_assocJoinReq(). The addition neighbor table holds every parent the
 * scan kept. */
static void print_ext_id(const u8 *id) {
    s8 i;

    for (i = 7; i >= 0; i--) {
        printf("%02x", id[i]);
    }
}

static const char *discovered_parent_kind(u8 device_type, u16 short_addr) {
    if (device_type == NWK_DEVICE_TYPE_COORDINATOR || short_addr == 0) {
        return "Coordinator";
    }
    if (device_type == NWK_DEVICE_TYPE_ROUTER) {
        return "Router";
    }
    return "End device";
}

static void print_discovered_parents(void) {
    u8 count = g_zb_neighborTbl.additionNeighborNum;
    u8 i;
    int best = -1;
    u8 best_lqi = 0;

    if (count > TL_ZB_ADDITION_NEIGHBOR_TABLE_SIZE) {
        count = TL_ZB_ADDITION_NEIGHBOR_TABLE_SIZE;
    }
    if (count == 0) {
        printf("No coordinators or routers heard\r\n");
        return;
    }

    for (i = 0; i < count; i++) {
        tl_zb_addition_neighbor_entry_t *entry =
            &g_zb_neighborTbl.additionNeighborTbl[i];
        if (entry->potentialParent && (best < 0 || entry->lqi > best_lqi)) {
            best     = i;
            best_lqi = entry->lqi;
        }
    }

    printf("Found %d parent%s\r\n", count, count == 1 ? "" : "s");
    for (i = 0; i < count; i++) {
        tl_zb_addition_neighbor_entry_t *entry =
            &g_zb_neighborTbl.additionNeighborTbl[i];
        int any = 0;

        printf("%s addr 0x%04x pan 0x%04x ch %d LQI %d extPan ",
               discovered_parent_kind(entry->deviceType, entry->shortAddr),
               entry->shortAddr, entry->panId, entry->logicChannel, entry->lqi);
        print_ext_id(entry->extPanId);
        printf("\r\n");

        if (i == (u8)best) {
            printf("  associating\r\n");
            continue;
        }

        printf("  not associating:");
        if (!entry->permitJoining) {
            printf(" joining closed");
            any = 1;
        }
#if ZB_ROUTER_ROLE
        if (!entry->routerCapacity) {
            printf(" no router capacity");
            any = 1;
        }
#else
        if (!entry->edCapacity) {
            printf(" no end-device capacity");
            any = 1;
        }
#endif
        if (entry->lqi < NWK_NEIGHBORTBL_ADD_LQITHRESHOLD) {
            printf(" LQI below %d", NWK_NEIGHBORTBL_ADD_LQITHRESHOLD);
            any = 1;
        }
        if (entry->deviceType == NWK_DEVICE_TYPE_ED) {
            printf(" end device");
            any = 1;
        }
        if (entry->stackProfile != ZB_STACK_PROFILE) {
            printf(" stack profile %d", entry->stackProfile);
            any = 1;
        }
        if (entry->depth >= NWK_MAX_DEPTH) {
            printf(" depth %d", entry->depth);
            any = 1;
        }
        if (entry->potentialParent && best >= 0) {
            tl_zb_addition_neighbor_entry_t *stronger =
                &g_zb_neighborTbl.additionNeighborTbl[best];
            printf(" weaker than 0x%04x LQI %d", stronger->shortAddr,
                   stronger->lqi);
            any = 1;
        }
        if (!any) {
            printf(" not marked as a potential parent");
        }
        printf("\r\n");
    }
}

u8 zb_assocJoinReq_after_discovery(void) {
    u8 status;

    print_discovered_parents();
    status = zb_assocJoinReq();
    if (status != ZDO_SUCCESS) {
        printf("Association request status %d\r\n", status);
    }
    return status;
}
#endif

void telink_zigbee_hal_network_init(void) {
    zb_init();
    zb_zdoCbRegister(&zdo_callbacks);
    af_powerDescPowerModeUpdate(POWER_MODE_RECEIVER_COMES_PERIODICALLY);
}

void telink_zigbee_hal_bdb_init(af_simple_descriptor_t *endpoint_descriptor) {
    // BDB init
    // TODO: Support from restore from deep sleep here
    bdb_init(endpoint_descriptor, &bdb_commission_setting, &device_bdb_cb, 1);
}
