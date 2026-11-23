#pragma once

#pragma pack(push, 1)
#include "zcl_include.h"
#pragma pack(pop)

status_t zcl_windowCovering_custom_register(u8 endpoint, u16 manuCode,
                                            u8 attrNum,
                                            const zclAttrInfo_t attrTbl[],
                                            cluster_forAppCb_t cb);
