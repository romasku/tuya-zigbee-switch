#include <stdint.h>
#include <stdbool.h>
#include "hal/tasks.h"

typedef struct
{
  uint8_t debounce;
  uint8_t step_ammount;

  uint32_t time_last_command_sent;

  bool is_command_scheduled;
  int scheduled_change;

  hal_task_t                       update_task;
  
} step_command_handler_t;

void setup_step_command_handler(step_command_handler_t *step_command_handler);

void step_command_handler_step_up(step_command_handler_t *step_command_handler);

//void step_command_handler_step_down(step_command_handler *step_command_handler);