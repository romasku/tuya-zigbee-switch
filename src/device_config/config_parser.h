#ifndef _DEVICE_INIT_H_
#define _DEVICE_INIT_H_

#include "base_components/network_indicator.h"
#include "hal/zigbee.h"

#include "config_nv.h"

extern network_indicator_t network_indicator;

extern hal_zigbee_endpoint endpoints[10];

void parse_config();
void init_reporting();
void handle_version_changes();

#endif
