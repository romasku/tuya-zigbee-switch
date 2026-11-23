#ifndef _DEVICE_INIT_H_
#define _DEVICE_INIT_H_

#include "base_components/network_indicator.h"
#include "base_components/battery.h"
#include "hal/zigbee.h"

#include "config_nv.h"

extern network_indicator_t network_indicator;

#define APP_ENDPOINT_LIMIT    16

extern hal_zigbee_endpoint endpoints[APP_ENDPOINT_LIMIT];

extern uint8_t allow_simultaneous_latching_pulses;

extern battery_t battery;

void parse_config();
void init_reporting();
void handle_version_changes();

#endif
