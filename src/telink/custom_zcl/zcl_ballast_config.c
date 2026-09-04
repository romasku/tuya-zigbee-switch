#pragma pack(push, 1)
#include "zcl_include.h"
#pragma pack(pop)

_CODE_ZCL_ status_t zcl_ballast_config_register(
    u8 endpoint, u16 manuCode, u8 attrNum, const zclAttrInfo_t attrTbl[],
    cluster_forAppCb_t cb)
{
  return (zcl_registerCluster(endpoint, 0x0301,
                              manuCode, attrNum, attrTbl, NULL, cb));
}
