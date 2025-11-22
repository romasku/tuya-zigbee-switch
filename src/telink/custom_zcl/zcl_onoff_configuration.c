#pragma pack(push, 1)
#include "zcl_include.h"
#pragma pack(pop)

_CODE_ZCL_ status_t zcl_onoff_configuration_register(
    u8 endpoint, u16 manuCode, u8 attrNum, const zclAttrInfo_t attrTbl[],
    cluster_forAppCb_t cb) {
  return (zcl_registerCluster(endpoint, ZCL_CLUSTER_GEN_ON_OFF_SWITCH_CONFIG,
                              manuCode, attrNum, attrTbl, NULL, cb));
}