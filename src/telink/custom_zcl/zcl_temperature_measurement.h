#ifndef ZCL_TEMPERATURE_MEASUREMENT_H
#define ZCL_TEMPERATURE_MEASUREMENT_H

#include "zcl_include.h"

status_t zcl_temperature_measurement_register(u8 endpoint, u16 manuCode,
                                              u8 attrNum,
                                              const zclAttrInfo_t attrTbl[],
                                              cluster_forAppCb_t cb);

#endif
