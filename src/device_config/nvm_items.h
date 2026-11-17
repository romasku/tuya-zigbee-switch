#ifndef DEVICE_CONFIG_NVM_ITEMS_H_
#define DEVICE_CONFIG_NVM_ITEMS_H_

#define MAX_RELAYS                       6
#define MAX_SWITCHES                     5
#define MAX_COVER_SWITCHES               3
#define MAX_COVERS                       3

#define NV_ITEM_CURRENT_VERSION_IN_NV    1
#define NV_ITEM_DEVICE_CONFIG            2
#define NV_ITEM_BASIC_CLUSTER_DATA       3
// switch_idx and relay_idx below are zero indexes, e.g. first switch has
// switch_idx = 0
#define NV_ITEM_SWITCH_CLUSTER_DATA(switch_idx) \
        (NV_ITEM_BASIC_CLUSTER_DATA + 1 + switch_idx)
// The NV item ids below are frozen: a device updated over the air keeps its
// NV, so moving an id makes it read another item's data. The id layout was
// sized for 5 relays; relays beyond that get ids after the fixed block.
#define NV_LAYOUT_RELAYS    5
#define NV_ITEM_RELAY_CLUSTER_DATA(relay_idx)                            \
        ((relay_idx) < NV_LAYOUT_RELAYS                                  \
         ? (NV_ITEM_BASIC_CLUSTER_DATA + MAX_SWITCHES + 1 + (relay_idx)) \
         : (NV_ITEM_EXTRA_RELAY_CLUSTER_DATA + (relay_idx) - NV_LAYOUT_RELAYS))
#define NV_ITEM_COVER_SWITCH_CONFIG(cover_switch_idx) \
        (NV_ITEM_BASIC_CLUSTER_DATA + MAX_SWITCHES + NV_LAYOUT_RELAYS + 1 + cover_switch_idx)
#define NV_ITEM_COVER_CONFIG(cover_idx)                                                          \
        (NV_ITEM_BASIC_CLUSTER_DATA + MAX_SWITCHES + NV_LAYOUT_RELAYS + MAX_COVER_SWITCHES + 1 + \
         cover_idx)

// 3 + 5 (switches) + 5 (relays) + 3 (cover switches) + 3 (covers) = 19
// Adding room for future items, so starting from 32
#define NV_ITEM_DEVICE_TYPE                 32

#define NV_ITEM_MULTI_PRESS_RESET_COUNT     33
#define NV_ITEM_POLL_CONTROL_CONFIG         34
#define NV_ITEM_DP_CONFIG                   35
#define NV_ITEM_DEVICE_CONFIG_EXT           36
// Relays 6.. (MAX_RELAYS - NV_LAYOUT_RELAYS of them)
#define NV_ITEM_EXTRA_RELAY_CLUSTER_DATA    37

#endif /* DEVICE_CONFIG_NVM_ITEMS_H_ */
