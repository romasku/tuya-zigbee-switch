#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)

nv_sts_t nv_resetToFactoryNew(void) {
  if (!nv_facrotyNewRstFlagCheck()) {
    nv_facrotyNewRstFlagSet();
  }

  foreach (i, NV_MAX_MODULS) {
    if (i != NV_MODULE_NWK_FRAME_COUNT && i != NV_MODULE_APP) {
      nv_resetModule(i);
    }
  }

  nv_facrotyNewRstFlagClear();
  return NV_SUCC;
}