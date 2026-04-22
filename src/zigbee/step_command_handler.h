#include <stdint.h>
#include <stdbool.h>
#include "hal/tasks.h"
#include "hal/zigbee.h"

typedef void (*zigbee_command_callback_t)();

typedef struct
{
  uint32_t debounce;
  uint16_t step_ammount;

  int time_last_command_sent;

  bool is_command_scheduled;
  int scheduled_change;

  hal_task_t update_task;

  void * sent_zigbee_command_param;
  zigbee_command_callback_t sent_zigbee_command;
  
} step_command_handler_t;

void setup_step_command_handler(step_command_handler_t *step_command_handler);

void step_command_handler_step_up(step_command_handler_t *step_command_handler);

//void step_command_handler_step_down(step_command_handler *step_command_handler);