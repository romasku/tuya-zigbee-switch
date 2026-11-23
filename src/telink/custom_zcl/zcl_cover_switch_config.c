#pragma pack(push, 1)
#include "zcl_include.h"
#pragma pack(pop)

_CODE_ZCL_ status_t zcl_cover_switch_config_register(
    u8 endpoint, u16 manuCode, u8 attrNum, const zclAttrInfo_t attrTbl[],
    cluster_forAppCb_t cb) {
    return(zcl_registerCluster(endpoint, 0xFC01,
                               manuCode, attrNum, attrTbl, NULL, cb));
}
