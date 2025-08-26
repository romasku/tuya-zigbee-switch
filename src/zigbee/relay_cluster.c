#include "tl_common.h"
#include "zb_common.h"
#include "endpoint.h"
#include "relay_cluster.h"
#include "cluster_common.h"
#include "configs/nv_slots_cfg.h"
#include "custom_zcl/zcl_onoff_indicator.h"



status_t relay_cluster_callback(zigbee_relay_cluster *cluster, zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t relay_cluster_callback_trampoline(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);

void relay_cluster_on_relay_change(zigbee_relay_cluster *cluster, u8 state);
void relay_cluster_on_write_attr(zigbee_relay_cluster *cluster, zclWriteCmd_t *pWriteReqCmd);

void relay_cluster_store_attrs_to_nv(zigbee_relay_cluster *cluster);
void relay_cluster_load_attrs_from_nv(zigbee_relay_cluster *cluster);
void relay_cluster_handle_startup_mode(zigbee_relay_cluster *cluster);

void sync_indicator_led(zigbee_relay_cluster *cluster);

zigbee_relay_cluster *relay_cluster_by_endpoint[10];

void relay_cluster_callback_attr_write_trampoline(u8 clusterId, zclWriteCmd_t *pWriteReqCmd)
{
  relay_cluster_on_write_attr(relay_cluster_by_endpoint[clusterId], pWriteReqCmd);
}

void update_relay_clusters()
{
  for (int i = 0; i < 10; i++)
  {
    if (relay_cluster_by_endpoint[i] != NULL)
    {
      sync_indicator_led(relay_cluster_by_endpoint[i]);
    }
  }
}

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster, zigbee_endpoint *endpoint)
{
  relay_cluster_by_endpoint[endpoint->index] = cluster;
  cluster->endpoint = endpoint->index;
  relay_cluster_load_attrs_from_nv(cluster);

#ifdef INDICATOR_PWM_SUPPORT
  pwm_nv_config_t pwm_config;
  if (pwm_nv_read_config(endpoint->index, &pwm_config))
  {
    cluster->pwm_enabled = pwm_config.pwm_enabled;
    cluster->pwm_brightness = pwm_config.pwm_brightness;
  }
  else
  {
    cluster->pwm_enabled = 0;
    cluster->pwm_brightness = led_pwm_get_default_brightness(endpoint->index);
  }
  cluster->pwm_saved_state = 0;
#endif

  cluster->relay->callback_param = cluster;
  cluster->relay->on_change      = (ev_relay_callback_t)relay_cluster_on_relay_change;

  relay_cluster_handle_startup_mode(cluster);
  sync_indicator_led(cluster);

  SETUP_ATTR(0, ZCL_ATTRID_ONOFF, ZCL_DATA_TYPE_BOOLEAN, ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE, cluster->relay->on);
  SETUP_ATTR(1, ZCL_ATTRID_START_UP_ONOFF, ZCL_DATA_TYPE_ENUM8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->startup_mode);
  if (cluster->indicator_led != NULL)
  {
    SETUP_ATTR(2, ZCL_ATTRID_ONOFF_INDICATOR_MODE, ZCL_DATA_TYPE_ENUM8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->indicator_led_mode);
    SETUP_ATTR(3, ZCL_ATTRID_ONOFF_INDICATOR_STATE, ZCL_DATA_TYPE_BOOLEAN, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, cluster->indicator_led->on);
  }

  zigbee_endpoint_add_cluster(endpoint, 1, ZCL_CLUSTER_GEN_ON_OFF);
  zcl_specClusterInfo_t *info = zigbee_endpoint_reserve_info(endpoint);
  info->clusterId           = ZCL_CLUSTER_GEN_ON_OFF;
  info->manuCode            = MANUFACTURER_CODE_NONE;
  info->attrNum             = cluster->indicator_led != NULL ? 4 : 2;
  info->attrTbl             = cluster->attr_infos;
  info->clusterRegisterFunc = zcl_onOff_register;
  info->clusterAppCb        = relay_cluster_callback_trampoline;
}

status_t relay_cluster_callback_trampoline(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
  return(relay_cluster_callback(relay_cluster_by_endpoint[pAddrInfo->dstEp], pAddrInfo, cmdId, cmdPayload));
}

status_t relay_cluster_callback(zigbee_relay_cluster *cluster, zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
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
  else
  {
    printf("Unknown command: %d\r\n", cmdId);
  }

  return(ZCL_STA_SUCCESS);
}

void sync_indicator_led(zigbee_relay_cluster *cluster)
{
  if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_MANUAL)
  {
    return;
  }
  
  if (cluster->indicator_led != NULL)
  {
    if (cluster->indicator_led->blink_times_left > 0)
    {
      return;
    }
    
#ifdef INDICATOR_PWM_SUPPORT
    static u16 prev_blink_times[10] = {0};
    if (prev_blink_times[cluster->endpoint] > 0 && cluster->indicator_led->blink_times_left == 0)
    {
      relay_cluster_restore_pwm_state_after_blink(cluster);
    }
    prev_blink_times[cluster->endpoint] = cluster->indicator_led->blink_times_left;
#endif
    
    u8 turn_on_led = cluster->relay->on;
    if (cluster->indicator_led_mode == ZCL_ONOFF_INDICATOR_MODE_OPPOSITE)
    {
      turn_on_led = !turn_on_led;
    }
    
#ifdef INDICATOR_PWM_SUPPORT
    if (cluster->pwm_enabled && turn_on_led && cluster->pwm_brightness > 0)
    {
      u8 led_index = 0;
      for (u8 i = 0; i < leds_cnt; i++)
      {
        if (&leds[i] == cluster->indicator_led)
        {
          led_index = i;
          break;
        }
      }
      
      led_pwm_enable(led_index, cluster->pwm_brightness);
    }
    else
#endif
    {
#ifdef INDICATOR_PWM_SUPPORT
      if (cluster->pwm_enabled)
      {
        u8 led_index = 0;
        for (u8 i = 0; i < leds_cnt; i++)
        {
          if (&leds[i] == cluster->indicator_led)
          {
            led_index = i;
            break;
          }
        }
        led_pwm_disable(led_index);
      }
#endif
      
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
}

void relay_cluster_on(zigbee_relay_cluster *cluster)
{
  relay_on(cluster->relay);
  sync_indicator_led(cluster);
  relay_cluster_report(cluster);
}

void relay_cluster_off(zigbee_relay_cluster *cluster)
{
  relay_off(cluster->relay);
  sync_indicator_led(cluster);
  relay_cluster_report(cluster);
}

void relay_cluster_toggle(zigbee_relay_cluster *cluster)
{
  relay_toggle(cluster->relay);
  sync_indicator_led(cluster);
  relay_cluster_report(cluster);
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
    if (pWriteReqCmd->attrList[index].attrID == ZCL_ATTRID_ONOFF_INDICATOR_STATE)
    {
      if (cluster->indicator_led->on)
      {
        led_on(cluster->indicator_led);
      }
      else
      {
        led_off(cluster->indicator_led);
      }
    }
  }
  if (cluster->indicator_led_mode != ZCL_ONOFF_INDICATOR_MODE_MANUAL)
  {
    sync_indicator_led(cluster);
  }

  relay_cluster_store_attrs_to_nv(cluster);
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
  nv_config_buffer.on_off             = cluster->relay->on;
  nv_config_buffer.startup_mode       = cluster->startup_mode;
  nv_config_buffer.indicator_led_mode = cluster->indicator_led_mode;
  nv_config_buffer.indicator_led_on   = cluster->indicator_led->on;

  nv_flashWriteNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_RELAY_CONFIG(cluster->endpoint), sizeof(zigbee_relay_cluster_config), (u8 *)&nv_config_buffer);
}

void relay_cluster_load_attrs_from_nv(zigbee_relay_cluster *cluster)
{
  nv_sts_t st = nv_flashReadNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_RELAY_CONFIG(cluster->endpoint), sizeof(zigbee_relay_cluster_config), (u8 *)&nv_config_buffer);

  if (st != NV_SUCC)
  {
    return;
  }
  cluster->startup_mode       = nv_config_buffer.startup_mode;
  cluster->indicator_led_mode = nv_config_buffer.indicator_led_mode;
}

void relay_cluster_handle_startup_mode(zigbee_relay_cluster *cluster)
{
  nv_sts_t st = nv_flashReadNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_RELAY_CONFIG(cluster->endpoint), sizeof(zigbee_relay_cluster_config), (u8 *)&nv_config_buffer);

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

void relay_cluster_led_blink(zigbee_relay_cluster *cluster, u16 on_time_ms, u16 off_time_ms, u16 times)
{
  if (cluster->indicator_led == NULL)
  {
    return;
  }
  
#ifdef INDICATOR_PWM_SUPPORT
  relay_cluster_save_pwm_state_for_blink(cluster);
#endif
  
  led_blink(cluster->indicator_led, on_time_ms, off_time_ms, times);
}

#ifdef INDICATOR_PWM_SUPPORT

void relay_cluster_set_pwm_brightness(zigbee_relay_cluster *cluster, u8 brightness)
{
  if (brightness > 15)
  {
    brightness = 15;
  }
  
  cluster->pwm_brightness = brightness;
  
  // Save to NV storage
  pwm_nv_config_t config;
  config.pwm_enabled = cluster->pwm_enabled;
  config.pwm_brightness = cluster->pwm_brightness;
  pwm_nv_write_config(cluster->endpoint, &config);
  
  if (cluster->pwm_enabled)
  {
    sync_indicator_led(cluster);
  }
}

u8 relay_cluster_get_pwm_brightness(zigbee_relay_cluster *cluster)
{
  return cluster->pwm_brightness;
}

void relay_cluster_enable_pwm(zigbee_relay_cluster *cluster, u8 enable)
{
  cluster->pwm_enabled = enable ? 1 : 0;
  
  // Save to NV storage
  pwm_nv_config_t config;
  config.pwm_enabled = cluster->pwm_enabled;
  config.pwm_brightness = cluster->pwm_brightness;
  pwm_nv_write_config(cluster->endpoint, &config);
  
  sync_indicator_led(cluster);
}

u8 relay_cluster_is_pwm_enabled(zigbee_relay_cluster *cluster)
{
  return cluster->pwm_enabled;
}

static u8 relay_cluster_get_led_index(zigbee_relay_cluster *cluster)
{
  if (cluster->indicator_led == NULL)
  {
    return 0xFF;
  }
  
  for (u8 i = 0; i < leds_cnt; i++)
  {
    if (&leds[i] == cluster->indicator_led)
    {
      return i;
    }
  }
  
  return 0xFF;
}

void relay_cluster_save_pwm_state_for_blink(zigbee_relay_cluster *cluster)
{
  u8 led_index = relay_cluster_get_led_index(cluster);
  if (led_index != 0xFF && cluster->pwm_enabled)
  {
    cluster->pwm_saved_state = cluster->pwm_brightness;
    led_pwm_disable(led_index);
  }
}

void relay_cluster_restore_pwm_state_after_blink(zigbee_relay_cluster *cluster)
{
  u8 led_index = relay_cluster_get_led_index(cluster);
  if (led_index != 0xFF && cluster->pwm_enabled)
  {
    cluster->pwm_brightness = cluster->pwm_saved_state;
    sync_indicator_led(cluster);
  }
}

#endif