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
    /* State last pushed to bindings by relay_cluster_report(), and whether
     * it has pushed at all yet. Lets it tell "state changed" from "someone
     * echoed back what I just sent", which is what keeps two relays bound
     * to each other from looping ON/OFF forever. */
    uint8_t              has_pushed;
    uint8_t              last_pushed_state;
} zigbee_relay_cluster;

void relay_cluster_add_to_endpoint(zigbee_relay_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint);

void relay_cluster_on(zigbee_relay_cluster *cluster);
void relay_cluster_off(zigbee_relay_cluster *cluster);
void relay_cluster_toggle(zigbee_relay_cluster *cluster);

void relay_cluster_report(zigbee_relay_cluster *cluster);

void update_relay_clusters();

void relay_cluster_callback_attr_write_trampoline(uint8_t endpoint,
                                                  uint16_t attribute_id);

#endif
