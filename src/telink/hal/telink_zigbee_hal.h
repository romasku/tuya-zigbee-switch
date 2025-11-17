#pragma once

#include "hal/zigbee.h"
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"

// Shared constants
#define MAX_ENDPOINTS 8
#define MAX_IN_CLUSTERS 32
#define MAX_OUT_CLUSTERS 32
#define MAX_ATTRS 128
#define OTA_QUERY_INTERVAL 15 * 60 // 15 minutes

// Network module functions (implemented in zigbee_network.c)
void telink_zigbee_hal_network_init(void);
void telink_zigbee_hal_bdb_init(af_simple_descriptor_t *endpoint_descriptor);

// ZCL module functions (implemented in zigbee_zcl.c)
void telink_zigbee_hal_zcl_init(hal_zigbee_endpoint *endpoints,
                                uint8_t endpoints_cnt);
af_simple_descriptor_t *telink_zigbee_hal_zcl_get_descriptors(void);

void telink_gpio_hal_setup_wake_ups();