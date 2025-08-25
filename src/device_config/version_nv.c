#include "tl_common.h"
#include "configs/nv_slots_cfg.h"
#include "version_cfg.h"

#define UNKNOWN_VERSION 0

u16 read_version_in_nv() {
  u16 version;

  nv_sts_t st;
  st = nv_flashReadNew(1, NV_MODULE_APP, NV_ITEM_CURRENT_VERSION_IN_NV, sizeof(version), (u8 *)&version);
  if (st == NV_SUCC) {
    printf("read version form new location\r\n");
    return version;
  }

  // Device before version <= 19 used NV_MODULE_ZCL, try it
  st = nv_flashReadNew(1, NV_MODULE_ZCL, 53, sizeof(version), (u8 *)&version);
  if (st == NV_SUCC) {
    printf("read version from old location\r\n");
    return version;
  }
  return UNKNOWN_VERSION;
}

void write_version_to_nv(u16 version) {
  nv_sts_t st = nv_flashWriteNew(1, NV_MODULE_APP, NV_ITEM_CURRENT_VERSION_IN_NV, sizeof(version), (u8 *)&version);
  if (st != NV_SUCC)
  {
    printf("Failed to write lastSeenVersion to NV, st: %d\r\n", st);
  }
}

#define SWITCH_CLUSTER_DATA_SIZE_V19 8
#define RELAY_CLUSTER_DATA_SIZE_V19  4

void migrate_to_v20() {
  // Move data from NV_MODULE_ZCL to NV_MODULE_APP
  nv_sts_t st = 0;
  u8 buffer[8];
  int item_id = 45;
  u16 item_len = 0;

  // Migrate all switch clusters
  while (item_id < 49) {  
    st = nv_flashSingleItemSizeGet(NV_MODULE_ZCL, item_id, &item_len);
    if (st != NV_SUCC || item_len != SWITCH_CLUSTER_DATA_SIZE_V19) {
      break;
    }
    st = nv_flashReadNew(1, NV_MODULE_ZCL, item_id, SWITCH_CLUSTER_DATA_SIZE_V19, buffer);
    if (st == NV_SUCC) {
      nv_flashWriteNew(1, NV_MODULE_APP, NV_ITEM_SWITCH_CLUSTER_DATA(item_id - 45), SWITCH_CLUSTER_DATA_SIZE_V19, buffer);
    }
    nv_flashSingleItemRemove(NV_MODULE_ZCL, item_id, SWITCH_CLUSTER_DATA_SIZE_V19);
    item_id++;
  }
  int first_relay_item_id = item_id;
  // Migrate all relay clusters
  while (item_id < 53) {  
    st = nv_flashSingleItemSizeGet(NV_MODULE_ZCL, item_id, &item_len);
    if (st != NV_SUCC || item_len != RELAY_CLUSTER_DATA_SIZE_V19) {
      break;
    }
    st = nv_flashReadNew(1, NV_MODULE_ZCL, item_id, RELAY_CLUSTER_DATA_SIZE_V19, buffer);
    if (st == NV_SUCC) {
      nv_flashWriteNew(1, NV_MODULE_APP, NV_ITEM_RELAY_CLUSTER_DATA(item_id - first_relay_item_id), RELAY_CLUSTER_DATA_SIZE_V19, buffer);
    }
    nv_flashSingleItemRemove(NV_MODULE_ZCL, item_id, RELAY_CLUSTER_DATA_SIZE_V19);
    item_id++;
  }
  // Can't migrate basic cluster + device config, as they used same items as relays due to bug
  write_version_to_nv(20);
}

void handle_version_changes()
{
  u16 oldVersion = read_version_in_nv();
  u16 currentVersion = STACK_BUILD;

  printf("Old version: %d\r\n", oldVersion);
  printf("Current version: %d\r\n", currentVersion);

  if (oldVersion == currentVersion)
  {
    // Same version, nothing to do
    return;
  }

  if (oldVersion == UNKNOWN_VERSION) {
    // Either old device or it first boot after re-flash, just store version
    write_version_to_nv(currentVersion);
    return;
  }

  if (oldVersion < 20) {
    migrate_to_v20();
  }
}