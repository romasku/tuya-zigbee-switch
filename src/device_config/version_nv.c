#include "zcl_include.h"
#include "configs/nv_slots_cfg.h"

void handle_version_changes()
{
  u16 lastSeenVersion = 0;
  u16 currentVersion = STACK_BUILD;

  nv_sts_t st;
  st = nv_flashReadNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_LAST_SEEN_VERSION, sizeof(lastSeenVersion), (u8 *)&lastSeenVersion);

  printf("Last seen version: %d\r\n", lastSeenVersion);
  printf("Current   version: %d\r\n", currentVersion);

  if (lastSeenVersion == currentVersion)
  {
    // Same version, nothing to do
    return;
  }

  if (st != NV_SUCC)
  {
    // Device was running version <= 17 before the update
    // or
    // This is the first boot after a reset / fresh install (any version).

    printf("No last seen version found\r\n");

    // Do something if needed
  }

  // Do something if needed

  lastSeenVersion = currentVersion;
  st = nv_flashWriteNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_LAST_SEEN_VERSION, sizeof(lastSeenVersion), (u8 *)&lastSeenVersion);
}