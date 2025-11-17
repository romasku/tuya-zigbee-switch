#ifndef ZCL_MULTISTATE_INPUT_H
#define ZCL_MULTISTATE_INPUT_H

#include "zcl_include.h"

status_t zcl_multistate_input_register(u8 endpoint, u16 manuCode, u8 attrNum,
                                       const zclAttrInfo_t attrTbl[],
                                       cluster_forAppCb_t cb);

#endif /* ZCL_MULTISTATE_INPUT_H */
