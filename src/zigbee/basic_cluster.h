#ifndef _BASIC_CLUSTER_H_
#define _BASIC_CLUSTER_H_

#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"

#include "endpoint.h"
#include "base_components/relay.h"

typedef struct
{
  u8            deviceEnable;
  char          manuName[32];
  char          modelId[32];
  zclAttrInfo_t attr_infos[13];
} zigbee_basic_cluster;

void basic_cluster_populate(zigbee_basic_cluster *cluster);
void basic_cluster_add_to_endpoint(zigbee_basic_cluster *cluster, zigbee_endpoint *endpoint);

void basic_cluster_callback_attr_write_trampoline(u8 clusterId, zclWriteCmd_t *pWriteReqCmd);

#endif
