#ifndef _TIME_CLUSTER_H_
#define _TIME_CLUSTER_H_

#include "hal/zigbee.h"
#include <stdint.h>

/* genTime server, so the coordinator can hand this device a real clock.
 *
 * A secondary MCU asks the module for the time (Tuya serial command 0x24) and
 * uses it for anything scheduled. We used to answer with zeros -- epoch 1970 --
 * on the assumption it only mattered for countdowns. That assumption was never
 * checked, and it is the one difference that lines up with the touch zones
 * drifting on zigbee2mqtt but never on the Tuya gateway, which supplies real
 * time.
 *
 * There is no RTC here and no way to ask the coordinator for the time: the HAL
 * can only send to bindings. So the cluster is exposed as a server instead and
 * the coordinator writes into it, which is the direction zigbee2mqtt already
 * supports. Between writes the clock free-runs off the millisecond counter. */

typedef struct {
    hal_zigbee_attribute attr_infos[4];
} zigbee_time_cluster;

void time_cluster_add_to_endpoint(zigbee_time_cluster *cluster,
                                  hal_zigbee_endpoint *endpoint);

/** Called from the attribute write path when Time or LocalTime changes. */
void time_cluster_on_write_attr(uint16_t attribute_id);

/** Seconds since the Unix epoch, or 0 when the coordinator has never told us.
 *  Callers must treat 0 as "unknown" rather than as 1970. */
uint32_t zigbee_time_unix(void);

#endif
