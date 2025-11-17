#ifndef _BINDING_TABLE_H_
#define _BINDING_TABLE_H_

#include "aps_api.h"

#define BINDING_TABLE_FOR_EACH(cluster, endpoint, out_ptr) \
    for (int i__ = 0; i__ < APS_BINDING_TABLE_NUM; ++i__) \
        if (g_apsBindingTbl[i__].used && g_apsBindingTbl[i__].clusterId == (cluster) && g_apsBindingTbl[i__].srcEp == (endpoint) && ((out_ptr) = (&g_apsBindingTbl[i__])))

#endif