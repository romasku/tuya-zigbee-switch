#ifndef ZCL_RELATIVE_HUMIDITY_H
#define ZCL_RELATIVE_HUMIDITY_H

#include "zcl_include.h"

status_t zcl_relative_humidity_register(u8 endpoint, u16 manuCode, u8 attrNum,
                                        const zclAttrInfo_t attrTbl[],
                                        cluster_forAppCb_t cb);

#endif
