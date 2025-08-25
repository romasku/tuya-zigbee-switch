#ifndef _RELAY_CLUSTER_H_
#define _RELAY_CLUSTER_H_

#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"

#include "endpoint.h"
#include "base_components/relay.h"
#include "base_components/led.h"

typedef struct
{
  u8            relay_idx;
  u8            endpoint;
  u8            startup_mode;
  u8            indicator_led_mode;
  zclAttrInfo_t attr_infos[4];
  relay_t *     relay;
  led_t *       indicator_led;
} zigbee_relay_cluster;

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster, zigbee_endpoint *endpoint);

void relay_cluster_on(zigbee_relay_cluster *cluster);
void relay_cluster_off(zigbee_relay_cluster *cluster);
void relay_cluster_toggle(zigbee_relay_cluster *cluster);

void relay_cluster_report(zigbee_relay_cluster *cluster);

void update_relay_clusters();

void relay_cluster_callback_attr_write_trampoline(u8 clusterId, zclWriteCmd_t *pWriteReqCmd);

#endif
