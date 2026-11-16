#ifndef _RELAY_CLUSTER_H_
#define _RELAY_CLUSTER_H_

#include "base_components/led.h"
#include "base_components/relay.h"
#include <stdint.h>

#include "hal/timer.h"
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

#ifdef HAL_HAS_TIMER_64
    // Auto-off timer (ZCL On With Timed Off). 0 = no timer, 1 = armed.
    uint8_t              off_timer_active;
    // Deadline in the hal_millis_64() time domain.
    uint64_t             off_timer_deadline_ms;
#endif
} zigbee_relay_cluster;

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint);

void relay_cluster_on(zigbee_relay_cluster *cluster);
void relay_cluster_off(zigbee_relay_cluster *cluster);
void relay_cluster_toggle(zigbee_relay_cluster *cluster);

void relay_cluster_report(zigbee_relay_cluster *cluster);

void update_relay_clusters();

#ifdef HAL_HAS_TIMER_64
// Fire any expired auto-off timer. Call from app_task().
void relay_cluster_task(void);

#endif

void relay_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                  uint16_t attribute_id);

#endif
