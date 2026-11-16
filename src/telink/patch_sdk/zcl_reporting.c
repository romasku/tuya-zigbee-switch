#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#pragma pack(pop)

// The SDK reporting engine sends attribute reports without an APS ack, so a
// report lost on the way to the coordinator is never retransmitted and the
// coordinator keeps a stale value (e.g. a cover position). Request an APS ack,
// so the stack retries the report, like the Silicon Labs stack does by default.
// zcl_reporting.c is compiled with -Dzcl_reportAttrs=zcl_reportAttrs__acked.
status_t zcl_reportAttrs__acked(u8 srcEp, epInfo_t *pDstEpInfo, u8 disableDefaultRsp,
                                u8 direction, u8 seqNo, u16 manuCode, u16 clusterId,
                                zclReportCmd_t *pReportAttrs) {
    pDstEpInfo->txOptions |= APS_TX_OPT_ACK_TX;
    return zcl_reportAttrs(srcEp, pDstEpInfo, disableDefaultRsp, direction, seqNo,
                           manuCode, clusterId, pReportAttrs);
}
