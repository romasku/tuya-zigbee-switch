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
    NULL, // tc detects that the frame counter is near limit
};

// Telink BDB storage
static bdb_commissionSetting_t bdb_commission_setting = {
    .linkKey.tcLinkKey.keyType = SS_GLOBAL_LINK_KEY,
    .linkKey.tcLinkKey.key = (u8 *)tcLinkKeyCentralDefault,

    .linkKey.distributeLinkKey.keyType = MASTER_KEY,
    .linkKey.distributeLinkKey.key = (u8 *)linkKeyDistributedMaster,

    .linkKey.touchLinkKey.keyType = MASTER_KEY,
    .linkKey.touchLinkKey.key = (u8 *)touchLinkKeyMaster,

    .touchlinkEnable = 0,
    .touchlinkChannel = DEFAULT_CHANNEL,
    .touchlinkLqiThreshold = 0xA0,
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

void zdo_leave_indication_callback(nlme_leave_ind_t *pLeaveInd) {}

void zdo_leave_confirmation_callback(nlme_leave_cnf_t *pLeaveCnf) {
  notify_about_network_status_change();
}

void bdb_init_callback(u8 status, u8 joinedNetwork) {
  if (status == BDB_INIT_STATUS_SUCCESS) {
    if (joinedNetwork) {
      ota_queryStart(OTA_QUERY_INTERVAL);
    }
  } else {
    if (joinedNetwork) {
      zb_rejoinReqWithBackOff(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
    }
  }
  notify_about_network_status_change();
}

void bdb_commissioning_callback(u8 status, void *arg) {
  printf("BDB commissioning callback, status: %d\r\n", status);
  switch (status) {
  case BDB_COMMISSION_STA_SUCCESS:
    ota_queryStart(OTA_QUERY_INTERVAL);
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
  leaveReq.rejoin = 0;
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

// Internal interface functions

void telink_zigbee_hal_network_init(void) {
  zb_init();
  zb_zdoCbRegister(&zdo_callbacks);
  af_powerDescPowerModeUpdate(POWER_MODE_RECEIVER_COMES_WHEN_STIMULATED);
}

void telink_zigbee_hal_bdb_init(af_simple_descriptor_t *endpoint_descriptor) {
  // BDB init
  // TODO: Support from restore from deep sleep here
  bdb_init(endpoint_descriptor, &bdb_commission_setting, &device_bdb_cb, 1);
}