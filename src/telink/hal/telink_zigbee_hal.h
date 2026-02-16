#pragma once

#include "hal/zigbee.h"

#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#pragma pack(pop)

// Shared constants
#define MAX_ENDPOINTS         8
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

// Request a short active period (fast poll + no sleep) to ensure pending
// ZCL reports are transmitted before deep retention.  ED-only; no-op on router.
void telink_zigbee_hal_request_active_period(void);

// Called from the AF data-confirm callback when all pending reports have
// been acknowledged by the MAC layer.  Ends the active period so the device
// can re-enter deep sleep immediately.  ED-only.
void telink_zigbee_hal_end_active_period(void);
