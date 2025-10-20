#ifndef ZCL_ONOFF_CONFIGURATION_H
#define ZCL_ONOFF_CONFIGURATION_H

#include "zcl_include.h"

status_t zcl_onoff_configuration_register(u8 endpoint, u16 manuCode, u8 attrNum,
                                          const zclAttrInfo_t attrTbl[],
                                          cluster_forAppCb_t cb);

#endif /* ZCL_ONOFF_CONFIGURATION_H */
