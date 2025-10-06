#include "tl_common.h"
#include "zcl_include.h"
#include "zigbee/basic_cluster.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/switch_cluster.h"


#ifdef ZCL_REPORT

void device_zclCfgReportCmd(u8 endpoint, u16 clusterId, zclCfgReportCmd_t *pCfgReportCmd)
{
  for (u8 i = 0; i < ZCL_REPORTING_TABLE_NUM; i++)
  {
    reportCfgInfo_t *pEntry = &reportingTab.reportCfgInfo[i];
    if (pEntry->used && pEntry->clusterID == clusterId && pEntry->endPoint == endpoint)
    {
      pEntry->minIntCnt = 0;
      pEntry->maxIntCnt = 0;
    }
  }
}

#endif

void device_zclWriteReqCmd(u8 endpoint, u16 clusterId, zclWriteCmd_t *pWriteReqCmd)
{
  if (clusterId == ZCL_CLUSTER_GEN_ON_OFF_SWITCH_CONFIG)
  {
    switch_cluster_callback_attr_write_trampoline(endpoint);
  }
  if (clusterId == ZCL_CLUSTER_GEN_ON_OFF)
  {
    relay_cluster_callback_attr_write_trampoline(endpoint, pWriteReqCmd);
  }
  if (clusterId == ZCL_CLUSTER_GEN_BASIC)
  {
    basic_cluster_callback_attr_write_trampoline(endpoint, pWriteReqCmd);
  }
  if (clusterId == ZCL_CLUSTER_GEN_IDENTIFY)
  {
    gen_identify_callback_attr_write_trampoline(endpoint, pWriteReqCmd);
  }
}

void device_zclProcessIncomingMsg(zclIncoming_t *pInHdlrMsg)
{
  u16 cluster = pInHdlrMsg->msg->indInfo.cluster_id;

  switch (pInHdlrMsg->hdr.cmd)
  {
#ifdef ZCL_REPORT
  case ZCL_CMD_CONFIG_REPORT:
    device_zclCfgReportCmd(pInHdlrMsg->msg->indInfo.dst_ep, cluster, pInHdlrMsg->attrCmd);
    break;
  case ZCL_CMD_WRITE:
    device_zclWriteReqCmd(pInHdlrMsg->msg->indInfo.dst_ep, cluster, pInHdlrMsg->attrCmd);
    break;
#endif
  default:
    break;
  }
}
