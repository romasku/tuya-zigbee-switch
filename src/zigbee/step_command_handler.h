#ifndef _STEP_COMMAND_HANDLER_H_
#define _STEP_COMMAND_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "hal/tasks.h"
#include "hal/zigbee.h"

typedef void (*step_command_handler_callback_t)(void *arg, int change, uint16_t trans_time);

typedef struct
{
  int _scheduled_change;
  uint32_t _last_command_sent_time;

  step_command_handler_callback_t _callback;
  void * _callback_arg; 
} step_command_handler_t;

void new_step_command_handler(step_command_handler_t *);
void step_command_handler_register_callback(step_command_handler_t *, step_command_handler_callback_t, void *);
void step_command_handler_step_up(step_command_handler_t *);
void step_command_handler_step_down(step_command_handler_t *);

#endif