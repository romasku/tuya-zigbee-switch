#ifndef DEVICE_CONFIG_NVM_ITEMS_H_
#define DEVICE_CONFIG_NVM_ITEMS_H_

#define MAX_RELAYS 5
#define MAX_SWITCHES 5
#define MAX_COVERS 3

#define NV_ITEM_CURRENT_VERSION_IN_NV 1
#define NV_ITEM_DEVICE_CONFIG 2
#define NV_ITEM_BASIC_CLUSTER_DATA 3
// switch_idx and relay_idx below are zero indexes, e.g. first switch has
// switch_idx = 0
#define NV_ITEM_SWITCH_CLUSTER_DATA(switch_idx)                                \
  (NV_ITEM_BASIC_CLUSTER_DATA + 1 + switch_idx)
#define NV_ITEM_RELAY_CLUSTER_DATA(relay_idx)                                  \
  (NV_ITEM_BASIC_CLUSTER_DATA + MAX_SWITCHES + 1 + relay_idx)
#define NV_ITEM_COVER_CONFIG(cover_idx) \
  (NV_ITEM_BASIC_CLUSTER_DATA + MAX_SWITCHES + MAX_RELAYS + 1 + cover_idx)

// 3 + 5 (switches) + 5 (relays) + 3 (covers) = 16
// Adding room for future items, so starting from 32
#define NV_ITEM_DEVICE_TYPE 32

#endif /* DEVICE_CONFIG_NVM_ITEMS_H_ */
