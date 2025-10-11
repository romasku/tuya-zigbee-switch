#ifndef _ENDPOINT_H_
#define _ENDPOINT_H_


#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"

#define MAX_CLUSTERS_PER_ENDPOINT    10

typedef struct
{
  u8                     index;
  af_simple_descriptor_t simple_description;
  u8                     cluster_info_cnts;
  u16                    in_clusters[MAX_CLUSTERS_PER_ENDPOINT];
  u16                    out_clusters[MAX_CLUSTERS_PER_ENDPOINT];
  zcl_specClusterInfo_t  cluster_infos[MAX_CLUSTERS_PER_ENDPOINT];
} zigbee_endpoint;

void zigbee_endpoint_init(zigbee_endpoint *endpoint, u16 app_dev_id);
void zigbee_endpoint_register_self(zigbee_endpoint *endpoint);
void zigbee_endpoint_add_cluster(zigbee_endpoint *endpoint, u8 server, u16 cluster_type);
zcl_specClusterInfo_t * zigbee_endpoint_reserve_info(zigbee_endpoint *endpoint);

#endif
