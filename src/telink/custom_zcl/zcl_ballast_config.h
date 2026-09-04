#ifndef ZCL_BALLAST_CONFIG_H_
#define ZCL_BALLAST_CONFIG_H_

#include "zcl_include.h"

status_t zcl_ballast_config_register(u8 endpoint, u16 manuCode, u8 attrNum,
                                     const zclAttrInfo_t attrTbl[],
                                     cluster_forAppCb_t cb);

#endif
