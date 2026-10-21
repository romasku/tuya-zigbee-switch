#ifndef _RELAY_CLUSTER_H_
#define _RELAY_CLUSTER_H_

#include "base_components/led.h"
#include "base_components/relay.h"
#include <stdint.h>

#include "hal/zigbee.h"

typedef struct {
    uint8_t              relay_idx;
    uint8_t              endpoint;
    uint8_t              startup_mode;
    uint8_t              indicator_led_mode;
    hal_zigbee_attribute attr_infos[4];
    relay_t *            relay;
    led_t *              indicator_led;
    uint8_t              indicator_state;

    // Auto-OFF timer. 0 = no timer, 1 = armed.
    uint8_t              off_timer_active;
    // 64-bit deadline in hal_millis_64() time domain.
    uint64_t             off_timer_deadline_ms;
} zigbee_relay_cluster;

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint);

void relay_cluster_on(zigbee_relay_cluster *cluster);
void relay_cluster_off(zigbee_relay_cluster *cluster);
void relay_cluster_toggle(zigbee_relay_cluster *cluster);

void update_relay_clusters();

// Fire any expired auto-OFF timer. Call from app_task().
//
// Router only: the polling tick uses hal_millis_64() which is based
// on the 16MHz system clock. On end_device builds the chip enters
// PM_SLEEP_MODE_SUSPEND between polls and the system clock stops,
// so the timer cannot fire reliably. The end_device entry in
// relay_cluster_callback() is therefore a no-op; this function
// becomes a no-op too when END_DEVICE is defined.
void relay_cluster_task(void);

void relay_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                  uint16_t attribute_id);

#endif
