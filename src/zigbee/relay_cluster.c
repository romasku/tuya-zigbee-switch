#include "tl_common.h"
#include "zb_common.h"
#include "endpoint.h"
#include "relay_cluster.h"
#include "cluster_common.h"
#include "configs/nv_slots_cfg.h"
#include "custom_zcl/zcl_onoff_indicator.h"
#include "base_components/millis.h"



status_t relay_cluster_callback(zigbee_relay_cluster *cluster, zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t relay_cluster_callback_trampoline(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);

status_t identify_cluster_callback_trampoline(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t identify_cluster_callback(zigbee_relay_cluster *cluster, zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);


void relay_cluster_on_relay_change(zigbee_relay_cluster *cluster, u8 state);
void relay_cluster_on_write_attr(zigbee_relay_cluster *cluster, zclWriteCmd_t *pWriteReqCmd);
void gen_identify_callback_attr_write(zigbee_relay_cluster *cluster, zclWriteCmd_t *pWriteReqCmd);


void relay_cluster_store_attrs_to_nv(zigbee_relay_cluster *cluster);
void relay_cluster_load_attrs_from_nv(zigbee_relay_cluster *cluster);
void relay_cluster_handle_startup_mode(zigbee_relay_cluster *cluster);

void relay_cluster_on_with_timed_off(zigbee_relay_cluster *cluster, zcl_onoffCtrl_t on_off_control, u16 on_time, u16 off_wait_time);

void sync_indicator_led(zigbee_relay_cluster *cluster);

void relay_cluster_update_effect(zigbee_relay_cluster *cluster);

void relay_cluster_stop_effect(zigbee_relay_cluster *cluster);

static bool relay_cluster_control_phys_relay(zigbee_relay_cluster *cluster);

zigbee_relay_cluster *relay_cluster_by_endpoint[10];

void relay_cluster_callback_attr_write_trampoline(u8 clusterId, zclWriteCmd_t *pWriteReqCmd)
{
  relay_cluster_on_write_attr(relay_cluster_by_endpoint[clusterId], pWriteReqCmd);
}

void gen_identify_callback_attr_write_trampoline(u8 clusterId, zclWriteCmd_t *pWriteReqCmd)
{
  gen_identify_callback_attr_write(relay_cluster_by_endpoint[clusterId], pWriteReqCmd);
}

void update_relay_clusters() {
  for (int i =0; i < 10; i++) {
    if (relay_cluster_by_endpoint[i] != NULL) {
      sync_indicator_led(relay_cluster_by_endpoint[i]);
    }
  }
}

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster, zigbee_endpoint *endpoint)
{
  relay_cluster_by_endpoint[endpoint->index] = cluster;
  cluster->endpoint = endpoint->index;
  relay_cluster_load_attrs_from_nv(cluster);

  cluster->relay->callback_param = cluster;
  // cluster->relay->on_change      = (ev_relay_callback_t)relay_cluster_on_relay_change;
  cluster->relay->on_change = NULL;
  cluster->relay->poll_callback  = (poll_relay_callback_t)relay_cluster_update_effect;

  relay_cluster_handle_startup_mode(cluster); 
  sync_indicator_led(cluster);

  SETUP_ATTR(0, ZCL_ATTRID_ONOFF, ZCL_DATA_TYPE_BOOLEAN, ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE | ACCESS_CONTROL_SCENE, cluster->on_off);
  SETUP_ATTR(1, ZCL_ATTRID_ON_TIME, ZCL_DATA_TYPE_UINT16, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->on_wait_time);
  SETUP_ATTR(2, ZCL_ATTRID_OFF_WAIT_TIME, ZCL_DATA_TYPE_UINT16, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->off_wait_time);
  SETUP_ATTR(3, ZCL_ATTRID_START_UP_ONOFF, ZCL_DATA_TYPE_ENUM8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->startup_mode);
  if (cluster->indicator_led != NULL)
  {
    SETUP_ATTR(4, ZCL_ATTRID_ONOFF_INDICATOR_MODE, ZCL_DATA_TYPE_ENUM8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->indicator_led_mode);
    SETUP_ATTR(5, ZCL_ATTRID_ONOFF_INDICATOR_STATE, ZCL_DATA_TYPE_BOOLEAN, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->indicator_led_on);
  }

  zigbee_endpoint_add_cluster(endpoint, 1, ZCL_CLUSTER_GEN_ON_OFF);
  zcl_specClusterInfo_t *info = zigbee_endpoint_reserve_info(endpoint);
  info->clusterId           = ZCL_CLUSTER_GEN_ON_OFF;
  info->manuCode            = MANUFACTURER_CODE_NONE;
  info->attrNum             = cluster->indicator_led != NULL ? 6 : 4;
  info->attrTbl             = cluster->attr_infos;
  info->clusterRegisterFunc = zcl_onOff_register;
  info->clusterAppCb        = relay_cluster_callback_trampoline;

  // Identify stuff
  SETUP_ATTR_FOR_TABLE(cluster->identify_attr_infos, 0, ZCL_ATTRID_IDENTIFY_TIME, ZCL_DATA_TYPE_UINT16, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->identify_time);

  zigbee_endpoint_add_cluster(endpoint, 1, ZCL_CLUSTER_GEN_IDENTIFY);
  info = zigbee_endpoint_reserve_info(endpoint);
  info->clusterId = ZCL_CLUSTER_GEN_IDENTIFY;
  info->manuCode  = MANUFACTURER_CODE_NONE;
  info->attrNum   = 1;
  info->attrTbl   = cluster->identify_attr_infos;
  info->clusterRegisterFunc = zcl_identify_register;
  info->clusterAppCb        = identify_cluster_callback_trampoline;

  relay_cluster_stop_effect(cluster);
}

status_t relay_cluster_callback_trampoline(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
  return(relay_cluster_callback(relay_cluster_by_endpoint[pAddrInfo->dstEp], pAddrInfo, cmdId, cmdPayload));
}

status_t relay_cluster_callback(zigbee_relay_cluster *cluster, zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
  zcl_onoff_cmdPayload_t *payload = cmdPayload;

  if (cmdId == ZCL_CMD_ONOFF_ON)
  {
    relay_cluster_on(cluster);
  }
  else if (cmdId == ZCL_CMD_ONOFF_OFF)
  {
    relay_cluster_off(cluster);
  }
  else if (cmdId == ZCL_CMD_ONOFF_TOGGLE)
  {
    relay_cluster_toggle(cluster);
  }
  else if (cmdId == ZCL_CMD_OFF_WITH_EFFECT)
  {
    relay_cluster_off(cluster);
  }
  else if (cmdId == ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE)
  {
    relay_cluster_on(cluster);
  }
  else if (cmdId == ZCL_CMD_ON_WITH_TIMED_OFF)
  {
    u16 on_time = payload->onWithTimeOff.onTime;
    u16 off_time = payload->onWithTimeOff.offWaitTime;
    zcl_onoffCtrl_t ctrl = payload->onWithTimeOff.onOffCtrl;

    if (on_time == 0xffff || off_time == 0xffff) // values from 0x0000 to 0xfffe are allowed
    {
      return ZCL_STA_INVALID_VALUE;
    }

    relay_cluster_on_with_timed_off(cluster, ctrl, on_time, off_time);
  }
  else
  {
    printf("Unknown command: %d\r\n", cmdId);
    return ZCL_STA_UNSUP_CLUSTER_COMMAND;
  }

  return(ZCL_STA_SUCCESS);
}


status_t identify_cluster_callback_trampoline(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
  return identify_cluster_callback(relay_cluster_by_endpoint[pAddrInfo->dstEp], pAddrInfo, cmdId, cmdPayload);
}

static void identify_set(zigbee_relay_cluster *cluster, u16 time)
{
  if (time == 0)
  {
    if (cluster->identify_time)
    {
      relay_cluster_stop_effect(cluster);
    }
    return;
  }

  cluster->identify_time = time;
  cluster->next_state = NEXT_STATE_OFF;
  cluster->last_update = millis();
  cluster->on_time = 500;
  cluster->off_time = 500;

  relay_on(cluster->relay);
}

status_t identify_cluster_callback(zigbee_relay_cluster *cluster, zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
  u8 status = ZCL_STA_SUCCESS;
  zcl_identify_cmdPayload_t *payload = cmdPayload;

  switch (cmdId) {
  case ZCL_CMD_IDENTIFY:
  {
    identify_set(cluster, payload->identify.identifyTime);
  }
    break;
  case ZCL_CMD_IDENTIFY_QUERY:
    break;
  case ZCL_CMD_TRIGGER_EFFECT:
    // Maybe one day...
  default:
		status = ZCL_STA_UNSUP_CLUSTER_COMMAND;
    break;
  }

  return status;
}

void sync_indicator_led(zigbee_relay_cluster *cluster)
{
  if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_MANUAL)
  {
    return;
  }
  if (cluster->indicator_led != NULL && !relay_cluster_is_identifying(cluster))
  {
    u8 turn_on_led = relay_cluster_is_on(cluster);
    if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_OPPOSITE)
    {
      turn_on_led = !turn_on_led;
    }
    if (turn_on_led)
    {
      led_on(cluster->indicator_led);
    }
    else
    {
      led_off(cluster->indicator_led);
    }
  }
}

void relay_cluster_restore_relay_state(zigbee_relay_cluster *cluster)
{
  if (cluster->indicator_led)
  {
    sync_indicator_led(cluster);

    if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_MANUAL)
    {
      if (cluster->indicator_led_on)
      {
        led_on(cluster->indicator_led);
      }
      else
      {
        led_off(cluster->indicator_led);
      }
    }
  }
  else if (cluster->relay)
  {
    if (cluster->on_off)
    {
      relay_on(cluster->relay);
    }
    else
    {
      relay_off(cluster->relay);
    }
  }
}

static void relay_cluster_set_on_off(zigbee_relay_cluster *cluster, enum relay_state state)
{
  bool on = state == RELAY_ON ? true : false;
  cluster->on_off = on;

  if (relay_cluster_control_phys_relay(cluster))
  {
    if (on)
    {
      relay_on(cluster->relay);
    }
    else
    {
      relay_off(cluster->relay);
    }
  }

  sync_indicator_led(cluster);
  relay_cluster_report(cluster);
  relay_cluster_on_relay_change(cluster, on);
}

void relay_cluster_on(zigbee_relay_cluster *cluster)
{
  if (cluster->on_wait_time == 0)
  {
    cluster->off_wait_time = 0;
  }

  relay_cluster_set_on_off(cluster, RELAY_ON);
}

void relay_cluster_off(zigbee_relay_cluster *cluster)
{
  cluster->on_wait_time = 0;
  cluster->on_off_count_from = millis();

  relay_cluster_set_on_off(cluster, RELAY_OFF);
}

void relay_cluster_toggle(zigbee_relay_cluster *cluster)
{
  bool cluster_on = cluster->on_off;

  if (cluster_on)
  {
    relay_cluster_off(cluster);
  }
  else
  {
    relay_cluster_on(cluster);
  }
}

void relay_cluster_on_with_timed_off(zigbee_relay_cluster *cluster, zcl_onoffCtrl_t on_off_control, u16 on_time, u16 off_wait_time)
{
  bool cluster_on = cluster->on_off;

  if (on_off_control.bits.acceptOnlyWhenOn && !cluster_on)
  {
    return;
  }

  if (cluster->off_wait_time > 0 && !cluster_on)
  {
    if (off_wait_time < cluster->off_wait_time)
    {
      cluster->off_wait_time = off_wait_time;
      cluster->on_off_count_from = millis();
    }

    return;
  }

  if (on_time >= cluster->on_wait_time)
  {
    cluster->on_wait_time = on_time;
    cluster->on_off_count_from = millis();
  }

  cluster->off_wait_time = off_wait_time;

  if (!cluster_on)
  {
    relay_cluster_set_on_off(cluster, RELAY_ON);
  }
}

void relay_cluster_report(zigbee_relay_cluster *cluster)
{
  if (zb_isDeviceJoinedNwk())
  {
    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);

    dstEpInfo.profileId   = HA_PROFILE_ID;
    dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;

    zclAttrInfo_t *pAttrEntry;
    pAttrEntry = zcl_findAttribute(cluster->endpoint, ZCL_CLUSTER_GEN_ON_OFF, ZCL_ATTRID_ONOFF);
    zcl_sendReportCmd(cluster->endpoint, &dstEpInfo, TRUE, ZCL_FRAME_SERVER_CLIENT_DIR,
                      ZCL_CLUSTER_GEN_ON_OFF, pAttrEntry->id, pAttrEntry->type, pAttrEntry->data);
    if (cluster->indicator_led != NULL)
    {
      pAttrEntry = zcl_findAttribute(cluster->endpoint, ZCL_CLUSTER_GEN_ON_OFF, ZCL_ATTRID_ONOFF_INDICATOR_STATE);
      zcl_sendReportCmd(cluster->endpoint, &dstEpInfo, TRUE, ZCL_FRAME_SERVER_CLIENT_DIR,
                        ZCL_CLUSTER_GEN_ON_OFF, pAttrEntry->id, pAttrEntry->type, pAttrEntry->data);
    }
  }
}

void relay_cluster_on_relay_change(zigbee_relay_cluster *cluster, u8 state)
{
  if (cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TOGGLE ||
      cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TO_PREVIOUS)
  {
    relay_cluster_store_attrs_to_nv(cluster);
  }
}

void relay_cluster_on_write_attr(zigbee_relay_cluster *cluster, zclWriteCmd_t *pWriteReqCmd)
{
  for (int index = 0; index < pWriteReqCmd->numAttr; index++)
  {
    zclWriteRec_t attr = pWriteReqCmd->attrList[index];
    if (attr.attrID == ZCL_ATTRID_ONOFF_INDICATOR_STATE && !relay_cluster_is_identifying(cluster))
    {
      if (cluster->indicator_led_on)
      {
        led_on(cluster->indicator_led);
      }
      else
      {
        led_off(cluster->indicator_led);
      }
    }

    bool cluster_on = relay_cluster_is_on(cluster);

    if (attr.attrID == ZCL_ATTRID_ON_TIME && attr.dataType == ZCL_DATA_TYPE_UINT16)
    {
      if (!cluster_on)
      {
        cluster->on_wait_time = 0;
        return;
      }

      cluster->on_off_count_from = millis();
    }

    if (attr.attrID == ZCL_ATTRID_OFF_WAIT_TIME && attr.dataType == ZCL_DATA_TYPE_UINT16)
    {
      if (!cluster_on)
      {
        cluster->on_off_count_from = millis();
      }
    }
  }
  if (cluster->indicator_led_mode != ZCL_ONOFF_INDICATOR_MODE_MANUAL)
  {
    sync_indicator_led(cluster);
  }

  relay_cluster_store_attrs_to_nv(cluster);
}

void gen_identify_callback_attr_write(zigbee_relay_cluster *cluster, zclWriteCmd_t *pWriteReqCmd)
{
  for (int index = 0; index < pWriteReqCmd->numAttr; index++)
  {
    zclWriteRec_t attr = pWriteReqCmd->attrList[index];
    if (attr.attrID == ZCL_ATTRID_IDENTIFY_TIME && attr.dataType == ZCL_DATA_TYPE_UINT16)
    {
      u16 identify_time = BUILD_U16(attr.attrData[0], attr.attrData[1]);
      identify_set(cluster, identify_time);
    }
  }
}

typedef struct
{
  u8 on_off;
  u8 startup_mode;
  u8 indicator_led_mode;
  u8 indicator_led_on;
} zigbee_relay_cluster_config;


zigbee_relay_cluster_config nv_config_buffer;


void relay_cluster_store_attrs_to_nv(zigbee_relay_cluster *cluster)
{
  nv_config_buffer.on_off             = cluster->on_off;
  nv_config_buffer.startup_mode       = cluster->startup_mode;
  nv_config_buffer.indicator_led_mode = cluster->indicator_led_mode;
  nv_config_buffer.indicator_led_on   = cluster->indicator_led_on;

  nv_flashWriteNew(1, NV_MODULE_APP, NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx), sizeof(zigbee_relay_cluster_config), (u8 *)&nv_config_buffer);
}

void relay_cluster_load_attrs_from_nv(zigbee_relay_cluster *cluster)
{
  nv_sts_t st = nv_flashReadNew(1, NV_MODULE_APP, NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx), sizeof(zigbee_relay_cluster_config), (u8 *)&nv_config_buffer);

  if (st != NV_SUCC)
  {
    return;
  }
  cluster->startup_mode       = nv_config_buffer.startup_mode;
  cluster->indicator_led_mode = nv_config_buffer.indicator_led_mode;
}

void relay_cluster_handle_startup_mode(zigbee_relay_cluster *cluster)
{
  nv_sts_t st = nv_flashReadNew(1, NV_MODULE_APP, NV_ITEM_RELAY_CLUSTER_DATA(cluster->relay_idx), sizeof(zigbee_relay_cluster_config), (u8 *)&nv_config_buffer);

  if (st != NV_SUCC)
  {
    return;
  }

  u8 prev_on = nv_config_buffer.on_off;
  if (cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF)
  {
    relay_cluster_off(cluster);
  }
  if (cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TO_ON)
  {
    relay_cluster_on(cluster);
  }
  if (cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TOGGLE)
  {
    if (prev_on)
    {
      relay_cluster_off(cluster);
    }
    else
    {
      relay_cluster_on(cluster);
    }
  }
  if (cluster->startup_mode == ZCL_START_UP_ONOFF_SET_ONOFF_TO_PREVIOUS)
  {
    if (prev_on)
    {
      relay_cluster_on(cluster);
    }
    else
    {
      relay_cluster_off(cluster);
    }
  }

  // Restore indicator led
  if (cluster->indicator_led != NULL)
  {
    if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_MANUAL)
    {
      cluster->indicator_led_on = nv_config_buffer.indicator_led_on;

      if (nv_config_buffer.indicator_led_on)
      {
        led_on(cluster->indicator_led);
      }
      else
      {
        led_off(cluster->indicator_led);
      }
    }
  }
}

void relay_cluster_stop_effect(zigbee_relay_cluster *cluster)
{
  cluster->toggles_left = 0;
  cluster->identify_time = 0;
  cluster->switch_counter = 0;

  relay_cluster_restore_relay_state(cluster);
}

static bool on_off_decrement(zigbee_relay_cluster *cluster, u32 current_time)
{
  bool relay_on = relay_cluster_is_on(cluster);

  if (relay_on)
  {
    return !(cluster->on_wait_time == 0 || cluster->on_wait_time == 0xffff) && ((cluster->on_off_count_from + 100) <= current_time);
  }
  else
  {
    return cluster->off_wait_time != 0 && ((cluster->on_off_count_from + 100) <= current_time);
  } 
}

void relay_cluster_update_timed_off(zigbee_relay_cluster *cluster)
{
  u32 time = millis();
  while (on_off_decrement(cluster, time))
  {
    if (relay_cluster_is_on(cluster))
    {
      cluster->on_wait_time -= 1;
      cluster->on_off_count_from += 100;
      if (cluster->on_wait_time == 0)
      {
        cluster->off_wait_time = 0;
        relay_cluster_off(cluster);
      }
    }
    else
    {
      cluster->off_wait_time -= 1;
      cluster->on_off_count_from += 100;
    }
  }
}

void relay_cluster_update_effect(zigbee_relay_cluster *cluster)
{
  // TODO
  relay_cluster_update_timed_off(cluster);

  if (!(cluster->identify_time) && !(cluster->toggles_left))
  {
    return;
  }

  u32 now = millis();
  u32 time_carry = now - cluster->last_update;

  cluster->last_update = now;
  while (time_carry >= cluster->switch_counter)
  {
    time_carry -= cluster->switch_counter;

    if (cluster->next_state == NEXT_STATE_OFF)
    {
      cluster->next_state = NEXT_STATE_ON;

      if (cluster->indicator_led)
      {
        led_off(cluster->indicator_led);
      }
      else if (cluster->relay)
      {
        relay_off(cluster->relay);
      }

      if (!cluster->identify_time)
      {
        cluster->toggles_left--;
        if (!cluster->toggles_left)
        {
          relay_cluster_stop_effect(cluster);
          return;
        }
      }
      cluster->switch_counter = cluster->off_time;
    }
    else
    {
      cluster->next_state = NEXT_STATE_OFF;

      if (cluster->indicator_led)
      {
        led_on(cluster->indicator_led);
      }
      else if (cluster->relay)
      {
        relay_on(cluster->relay);
      }

      if (cluster->identify_time)
      {
        cluster->identify_time--;
        if (!cluster->identify_time)
        {
          relay_cluster_stop_effect(cluster);
          return;
        }
      }
      else if (!(--cluster->toggles_left))
      {
        relay_cluster_stop_effect(cluster);
        return;
      }

      cluster->switch_counter = cluster->on_time;
    }
  }
  cluster->switch_counter -= time_carry;
}

bool relay_cluster_is_on(zigbee_relay_cluster *cluster)
{
  return cluster->on_off;
}

bool relay_cluster_is_identifying(zigbee_relay_cluster *cluster)
{
  return cluster->identify_time || cluster->toggles_left;
}

static bool relay_cluster_control_phys_relay(zigbee_relay_cluster *cluster)
{
  return !relay_cluster_is_identifying(cluster) || cluster->indicator_led;
}