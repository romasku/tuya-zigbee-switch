#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#pragma pack(pop)

#include "ota.h"

#include "telink_size_t_hack.h"

#include "hal/tasks.h"
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

#ifdef ZB_ED_ROLE
// After joining, stay active for coordinator to read descriptors/attributes,
// then switch to slow poll and allow sleep.
#define POST_JOIN_FAST_POLL_RATE    250   // 250ms polling during initial setup
#define POST_JOIN_SETTLE_MS         45000 // 45s before allowing sleep (Z2M interview + binding)

// After attribute change, stay awake briefly so the poll cycle can send the
// ZCL report before we enter deep retention.
// This is a safety timeout only — normally the data-confirm callback
// (telink_zigbee_hal_end_active_period) shortens the window to a brief
// cooldown (~200 ms) right after the MAC ACK, so the device sleeps much
// sooner than the full safety timeout.
#define REPORT_ACTIVE_TIME_MS       1500  // safety-net timeout (ms)
#define POST_CONFIRM_COOLDOWN_MS    200   // cooldown after last data confirm

// Power management is disabled until device successfully joins and settles
static bool pm_enabled = false;

static bool join_settling = false;
static u32 join_settle_timer = 0;

static bool report_active = false;
static u32 report_active_timer = 0;
static u32 report_active_timeout_ms = REPORT_ACTIVE_TIME_MS;

void telink_zigbee_hal_request_active_period(void) {
    if (report_active) {
        // Already active — if we're in post-confirm cooldown, extend back
        // to the full safety timeout to allow a new report to be sent.
        // Otherwise do nothing (avoid duplicate polls / timer resets).
        if (report_active_timeout_ms == POST_CONFIRM_COOLDOWN_MS) {
            report_active_timer = clock_time();
            report_active_timeout_ms = REPORT_ACTIVE_TIME_MS;
        }
        return;
    }
    report_active = true;
    report_active_timer = clock_time();
    report_active_timeout_ms = REPORT_ACTIVE_TIME_MS;
    if (!join_settling) {
        zb_setPollRate(QUEUE_POLL_RATE);
    }
    // Trigger an immediate MAC Data Request so the frame can go out
    // on the very next main-loop iteration instead of waiting for the
    // next poll-rate cycle.
    zb_endDeviceSyncReq();
    printf("[%d] Active period started\r\n",
           clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS);
}

void telink_zigbee_hal_end_active_period(void) {
    if (!report_active) {
        return; // No active period — ignore stray confirms (e.g. read responses)
    }
    // A frame was confirmed by the MAC layer.  Instead of ending the
    // active window immediately (another report may still be queued),
    // shorten the remaining window to POST_CONFIRM_COOLDOWN_MS.
    // If no further data is queued, the timer check in app_task will
    // clear report_active and allow sleep.
    report_active_timer = clock_time();
    report_active_timeout_ms = POST_CONFIRM_COOLDOWN_MS;
    printf("[%d] Data confirmed, cooldown %d ms\r\n",
           clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS,
           POST_CONFIRM_COOLDOWN_MS);
}

void hal_zigbee_check_settle_timer(void) {
    // Check if settling period is over (must be called regularly from app_task)
    if (join_settling) {
        u32 current_time = clock_time();
        u32 elapsed_ticks = current_time - join_settle_timer;
        u32 target_ticks = POST_JOIN_SETTLE_MS * CLOCK_16M_SYS_TIMER_CLK_1MS;
        
        if (elapsed_ticks >= target_ticks) {
            join_settling = false;
            pm_enabled = true; // Enable power management after settle
            zb_setPollRate(POLL_RATE); // Use configured long poll rate
            printf("[%d] Settle period ended: slow poll %d, PM enabled\r\n", 
                   current_time / CLOCK_16M_SYS_TIMER_CLK_1MS, POLL_RATE);
        }
    }
}

void hal_zigbee_check_report_active_timer(void) {
    // Expire the active window.  Two cases:
    //  1. Safety-net: REPORT_ACTIVE_TIME_MS elapsed with no data confirm
    //     (parent unreachable, frame lost).
    //  2. Post-confirm cooldown: POST_CONFIRM_COOLDOWN_MS after the last
    //     data confirm, giving time for any follow-up reports.
    if (report_active) {
        u32 current_time = clock_time();
        u32 elapsed_ticks = current_time - report_active_timer;
        u32 target_ticks = report_active_timeout_ms * CLOCK_16M_SYS_TIMER_CLK_1MS;

        if (elapsed_ticks >= target_ticks) {
            bool was_cooldown =
                (report_active_timeout_ms == POST_CONFIRM_COOLDOWN_MS);
            report_active = false;
            report_active_timeout_ms = REPORT_ACTIVE_TIME_MS;
            if (!join_settling) {
                zb_setPollRate(POLL_RATE); // Back to slow poll
            }
            printf("[%d] Active period ended (%s)\r\n",
                   current_time / CLOCK_16M_SYS_TIMER_CLK_1MS,
                   was_cooldown ? "cooldown" : "timeout");
        }
    }
}

bool hal_zigbee_is_sleep_allowed(void) {
    // Power management is disabled during commissioning and settling
    if (!pm_enabled) {
        return false;
    }
    
    // Block sleep during settling or reporting (timers checked in app_task)
    return !join_settling && !report_active;
}
#else
void telink_zigbee_hal_request_active_period(void) {
    // Router: always awake, no-op
}
#endif

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
            // Already joined (reboot): enable PM immediately, use slow poll
            pm_enabled = true;
            join_settling = false;
            zb_setPollRate(POLL_RATE);
            printf("[%d] Rejoined: slow poll %d, PM enabled\r\n", 
                   clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS, POLL_RATE);
      #endif
        } else {
      #ifdef ZB_ED_ROLE
            // Not joined: disable PM until join completes and settles
            pm_enabled = false;
            join_settling = false;
            printf("[%d] Not joined: PM disabled until configured\r\n", 
                   clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS);
      #endif
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
#ifdef ZB_ED_ROLE
        // Fast poll so coordinator can read descriptors/attributes after join
        // PM stays disabled during settle period
        join_settling = true;
        join_settle_timer = clock_time();
        zb_setPollRate(POST_JOIN_FAST_POLL_RATE);
        printf("[%d] Joined: fast poll %d, settle %ds (PM disabled)\r\n",
               clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS, POST_JOIN_FAST_POLL_RATE, POST_JOIN_SETTLE_MS / 1000);
        printf("[%d] DEBUG: join_settling=%d, timer=%u, target=%u ticks\r\n",
               clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS, join_settling, join_settle_timer,
               POST_JOIN_SETTLE_MS * CLOCK_16M_SYS_TIMER_CLK_1MS);
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
