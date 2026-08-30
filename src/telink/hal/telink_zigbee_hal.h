#pragma once

#include "hal/zigbee.h"

#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#pragma pack(pop)

// Shared constants
// Raised from 8: telink_zigbee_hal_zcl_init() silently truncates
// endpoints_cnt to this before registering with the stack, independent of
// MAX_ACTIVE_EP_NUMBER (zb_af.h, already 16) and of endpoints[13] in
// config_parser.c. A full-bind 6-gang board (6 ST + 6 RT = 12) was
// registering only its first 8 endpoints on real hardware -- z2m's
// Configure got as far as endpoint 8 and then found endpoints 9-12
// simply did not exist. Matches endpoints[13], the app's own ceiling.
#define MAX_ENDPOINTS         13
// Raised from 32: this is a flat, device-wide array (telink_zigbee_hal_zcl_init
// appends to it once per server cluster across every endpoint, no per-endpoint
// reset), separate from our own app-level clusters[64] pool in
// config_parser.c. A full-bind 6-gang board registers ~44 server clusters
// total (basic+ota+time+switch x6 on ep1..6, relay+group x6 on ep7..12) --
// past 32 the write runs off the end of in_clusters[], and past
// ZCL_CLUSTER_NUM_MAX (stack_cfg.h, raised alongside this) zcl_registerCluster
// starts silently refusing clusters, which is what produced
// UNSUPPORTED_ATTRIBUTE on the later relay endpoints.
#define MAX_IN_CLUSTERS       56
#define MAX_OUT_CLUSTERS      32
// Raised from 128: same device-wide-array reasoning as MAX_IN_CLUSTERS.
// Rough count for the 6-switch/6-relay layout comes out close to 128 on
// its own (~12 attrs/switch, ~6/relay, ~20 on the shared Basic cluster).
#define MAX_ATTRS             192
#define OTA_QUERY_INTERVAL    15 * 60 // 15 minutes

// Network module functions (implemented in zigbee_network.c)
void telink_zigbee_hal_network_init(void);
void telink_zigbee_hal_bdb_init(af_simple_descriptor_t *endpoint_descriptor);

// ZCL module functions (implemented in zigbee_zcl.c)
void telink_zigbee_hal_zcl_init(hal_zigbee_endpoint *endpoints,
                                uint8_t endpoints_cnt);
af_simple_descriptor_t *telink_zigbee_hal_zcl_get_descriptors(void);

void telink_gpio_hal_setup_wake_ups();

void telink_gpio_reinit_after_deep_retention();
void telink_gpio_to_pull_for_deep_retention();
void telink_gpio_reinit_interrupts();
