#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"
#include "endpoint.h"


void zigbee_endpoint_init(zigbee_endpoint *endpoint, u16 app_dev_id)
{
  endpoint->simple_description.app_profile_id      = HA_PROFILE_ID;
  endpoint->simple_description.app_dev_id          = app_dev_id;
  endpoint->simple_description.endpoint            = endpoint->index;
  endpoint->simple_description.app_dev_ver         = 1;
  endpoint->simple_description.reserved            = 0;
  endpoint->simple_description.app_in_cluster_lst  = endpoint->in_clusters;
  endpoint->simple_description.app_out_cluster_lst = endpoint->out_clusters;
}

void zigbee_endpoint_register_self(zigbee_endpoint *endpoint)
{
  af_endpointRegister(endpoint->index, (af_simple_descriptor_t *)&endpoint->simple_description, zcl_rx_handler, NULL);
  zcl_register(endpoint->index, endpoint->cluster_info_cnts, (zcl_specClusterInfo_t *)endpoint->cluster_infos);
}

void zigbee_endpoint_add_cluster(zigbee_endpoint *endpoint, u8 server, u16 cluster_type)
{
  if (server)
  {
    u8 index = endpoint->simple_description.app_in_cluster_count;
    endpoint->simple_description.app_in_cluster_lst[index] = (u16)cluster_type;
    index++;
    endpoint->simple_description.app_in_cluster_count = index;
  }
  else
  {
    u8 index = endpoint->simple_description.app_out_cluster_count;
    endpoint->simple_description.app_out_cluster_lst[index] = (u16)cluster_type;
    index++;
    endpoint->simple_description.app_out_cluster_count = index;
  }
}

zcl_specClusterInfo_t * zigbee_endpoint_reserve_info(zigbee_endpoint *endpoint)
{
  zcl_specClusterInfo_t *info = endpoint->cluster_infos + endpoint->cluster_info_cnts;

  endpoint->cluster_info_cnts++;
  return(info);
}
