#include <stdint.h>
#include <stdbool.h>
#include "hal/tasks.h"
#include "hal/zigbee.h"

typedef void (*step_command_handler_callback_t)(void *arg, int change, uint16_t trans_time);

typedef struct step_command_handler_2_t
{
  int _scheduled_change;

  step_command_handler_callback_t _callback;
  void * _callback_arg;


  void (*step_up) (struct step_command_handler_2_t*);
  void (*step_down) (struct step_command_handler_2_t*);
  void (*register_callback) (struct step_command_handler_2_t*,  step_command_handler_callback_t, void *);

 
} step_command_handler_2_t;

step_command_handler_2_t * new_step_command_handler();