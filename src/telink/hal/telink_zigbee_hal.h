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
#define MAX_IN_CLUSTERS       32
#define MAX_OUT_CLUSTERS      32
#define MAX_ATTRS             128
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
