#ifndef _COVER_CLUSTER_H_
#define _COVER_CLUSTER_H_

#include "base_components/relay.h"
#include "hal/zigbee.h"
#include "hal/tasks.h"
#include <stdint.h>

#define COVER_ACTION_NONE    0xFFFF         // No pending operation
#define COVER_ACTION_STOP    0xFFFE         // Stop the motor

/**
 * Represents an action to be performed on a cover. The action can be either:
 * - The target motor position between 0 and 10000
 * - COVER_ACTION_STOP to stop the motor immediately
 * - COVER_ACTION_NONE to indicate no pending action
 */
typedef uint16_t cover_action_t;

typedef struct {
    uint8_t  motor_reversal;
    uint16_t open_time;
    uint16_t close_time;
    uint16_t open_deadzone;
    uint16_t closed_deadzone;
    uint16_t motor_position;
} zigbee_cover_cluster_config;

typedef struct {
    // Parameters
    uint8_t              cover_idx;
    uint8_t              endpoint;
    relay_t *            open_relay;
    relay_t *            close_relay;

    // Attributes
    uint8_t              moving;
    uint8_t              motor_reversal;
    uint16_t             open_time;
    uint16_t             close_time;
    uint16_t             open_deadzone;
    uint16_t             closed_deadzone;
    uint8_t              position;
    hal_zigbee_attribute attr_infos[8];

    // State
    uint32_t             last_switch_time;
    cover_action_t       pending_action;
    uint16_t             motor_position;
    uint16_t             target_motor_position;
    uint32_t             movement_start_time;
    uint16_t             start_motor_position;
    hal_task_t           delay_task;
    hal_task_t           stop_task;
    hal_task_t           update_task;
} zigbee_cover_cluster;

void cover_cluster_add_to_endpoint(zigbee_cover_cluster *cluster, hal_zigbee_endpoint *endpoint);

void cover_open(zigbee_cover_cluster *cluster);
void cover_close(zigbee_cover_cluster *cluster);
void cover_stop(zigbee_cover_cluster *cluster);

void cover_cluster_callback_attr_write_trampoline(uint8_t endpoint, uint16_t attribute_id);

#endif
