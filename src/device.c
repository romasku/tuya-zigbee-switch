/**********************************************************************
 * INCLUDES
 */
#include "tl_common.h"
#include "device.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "bdb.h"
#include "ota.h"
#include "device.h"

#include "chip_8258/timer.h"
#include "ext_ota.h"
#include "reporting.h"

#include "base_components/millis.h"
#include "device_config/device_config.h"

#include "zigbee/general_commands.h"
#include "zigbee/bdb_callbacks.h"
#include "zigbee/endpoint.h"
#include "zigbee/basic_cluster.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/switch_cluster.h"
#include "zigbee/general.h"

#include "custom_zcl/zcl_onoff_configuration.h"
#include "custom_zcl/zcl_multistate_input.h"


/**********************************************************************
 * GLOBAL VARIABLES
 */
app_ctx_t g_baseAppCtx;

extern ota_callBack_t baseEndpoint_otaCb;

//running code firmware information
ota_preamble_t baseEndpoint_otaInfo =
{
  .fileVer          = FILE_VERSION,
  .imageType        = IMAGE_TYPE,
  .manufacturerCode = MANUFACTURER_CODE_TELINK,
};


//Must declare the application call back function which used by ZDO layer
const zdo_appIndCb_t appCbLst =
{
  bdb_zdoStartDevCnf,     //start device cnf cb
  NULL,                   //reset cnf cb
  NULL,                   //device announce indication cb
  device_leaveIndHandler, //leave ind cb
  device_leaveCnfHandler, //leave cnf cb
  NULL,                   //nwk update ind cb
  NULL,                   //permit join ind cb
  NULL,                   //nlme sync cnf cb
  NULL,                   //tc join ind cb
  NULL,                   //tc detects that the frame counter is near limit
};


/**
 *  @brief Definition for bdb commissioning setting
 */
bdb_commissionSetting_t g_bdbCommissionSetting =
{
  .linkKey.tcLinkKey.keyType = SS_GLOBAL_LINK_KEY,
  .linkKey.tcLinkKey.key     = (u8 *)tcLinkKeyCentralDefault,                   //can use unique link key stored in NV

  .linkKey.distributeLinkKey.keyType = MASTER_KEY,
  .linkKey.distributeLinkKey.key     = (u8 *)linkKeyDistributedMaster,          //use linkKeyDistributedCertification before testing

  .linkKey.touchLinkKey.keyType = MASTER_KEY,
  .linkKey.touchLinkKey.key     = (u8 *)touchLinkKeyMaster,                             //use touchLinkKeyCertification before testing

  .touchlinkEnable       =                  0,                                          /* disable touch-link */
  .touchlinkChannel      = DEFAULT_CHANNEL,                                             /* touch-link default operation channel for target */
  .touchlinkLqiThreshold =               0xA0,                                          /* threshold for touch-link scan req/resp command */
};


/*********************************************************************
 * @fn      stack_init
 *
 * @brief   This function initialize the ZigBee stack and related profile. If HA/ZLL profile is
 *          enabled in this application, related cluster should be registered here.
 *
 * @param   None
 *
 * @return  None
 */
void stack_init(void)
{
  zb_init();
  zb_zdoCbRegister((zdo_appIndCb_t *)&appCbLst);
}

/*********************************************************************
 * @fn      user_app_init
 *
 * @brief   This function initialize the application(Endpoint) information for this node.
 *
 * @param   None
 *
 * @return  None
 */
void user_app_init(void)
{
  af_powerDescPowerModeUpdate(POWER_MODE_RECEIVER_COMES_WHEN_STIMULATED);

  /* Initialize ZCL layer */
  /* Register Incoming ZCL Foundation command/response messages */
  zcl_init(device_zclProcessIncomingMsg);

  handle_version_changes();

  parse_config();

  ota_init(OTA_TYPE_CLIENT, (af_simple_descriptor_t *)&endpoints[0].simple_description, &baseEndpoint_otaInfo, &baseEndpoint_otaCb);
}

bool boot_announce_sent = false;

void app_task(void)
{
  millis_update();
  periferals_update();
  if (bdb_isIdle())
  {
    if (zb_isDeviceJoinedNwk())
    {
      network_indicator_connected(&network_indicator);
      if (zb_isDeviceFactoryNew())
      {
        zb_deviceFactoryNewSet(false);
      }

      if (!boot_announce_sent)
      {
        // Send announcement to notify that device is up
        zb_zdoSendDevAnnance();
        boot_announce_sent = true;
      }

      // report handler
      if (g_baseAppCtx.lastReportCheckSec != seconds())
      {
        app_chk_report(seconds() - g_baseAppCtx.lastReportCheckSec);
        g_baseAppCtx.lastReportCheckSec = seconds();
      }

      update_relay_clusters();
    }
    else                 // Device not Joined
    {
      network_indicator_not_connected(&network_indicator);
    }
                #if PM_ENABLE
    if (!tl_stackBusy() && zb_isTaskDone())
    {
      drv_pm_sleep(PM_SLEEP_MODE_SUSPEND, PM_WAKEUP_SRC_TIMER, PM_SLEEP_DURATION_MS);
    }
                #endif
  }
}

/*********************************************************************
 * @fn      user_init
 *
 * @brief   User level initialization code.
 *
 * @param   isRetention - if it is waking up with ram retention.
 *
 * @return  None
 */
void user_init(bool isRetention)
{
  if (!isRetention)
  {
    /* Populate properties with compiled-in values */

    /* Initialize Stack */
    stack_init();

    /* Initialize user application */
    user_app_init();

    /* User's Task */
    ev_on_poll(EV_POLL_IDLE, app_task);

    /* Load the pre-install code from flash */
    if (bdb_preInstallCodeLoad(&g_baseAppCtx.tcLinkKey.keyType, g_baseAppCtx.tcLinkKey.key) == RET_OK)
    {
      g_bdbCommissionSetting.linkKey.tcLinkKey.keyType = g_baseAppCtx.tcLinkKey.keyType;
      g_bdbCommissionSetting.linkKey.tcLinkKey.key     = g_baseAppCtx.tcLinkKey.key;
    }
    /* Set default reporting configuration */
    init_reporting();

    /* Initialize BDB */
    u8 repower = drv_pm_deepSleep_flag_get() ? 0 : 1;
    bdb_init((af_simple_descriptor_t *)&endpoints[0].simple_description, &g_bdbCommissionSetting, &g_deviceBdbCb, repower);
  }
  else
  {
    /* Re-config phy when system recovery from deep sleep with retention */
    mac_phyReconfig();
  }
}
