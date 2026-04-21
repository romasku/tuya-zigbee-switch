#include <stdint.h>
#include "step_command_handler.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include "hal/printf_selector.h"

void send_command(void *arg) {
  step_command_handler_t *step_command_handler = (step_command_handler_t *)arg;

  printf("Sending command, to change by %i in %i ms\r\n", step_command_handler->scheduled_change, step_command_handler->debounce);

  step_command_handler->time_last_command_sent = hal_millis();
  step_command_handler->is_command_scheduled = false;
  step_command_handler->scheduled_change = 0;
}

void setup_step_command_handler(step_command_handler_t *step_command_handler) {
  step_command_handler->debounce = 50; // ms
  step_command_handler->step_ammount = 10;

  step_command_handler->time_last_command_sent = 0;
  step_command_handler->is_command_scheduled = false;
  step_command_handler->scheduled_change = 0;

  step_command_handler->update_task.handler = send_command;
  step_command_handler->update_task.arg     = step_command_handler;
  hal_tasks_init(&step_command_handler->update_task);
}

void step_command_handler_step_up(step_command_handler_t *step_command_handler) {

  uint32_t now = hal_millis();
  if(now > step_command_handler->time_last_command_sent + step_command_handler->debounce) {
    // Command has not been recently sent, sent command straight away
    step_command_handler->scheduled_change = step_command_handler->step_ammount;
    send_command(step_command_handler);

  } else {
    // Command has been sent recently
    
    if(!step_command_handler->is_command_scheduled) {
      // Schedule a command to be sent when the last should complete
      int last_command_finish_time = step_command_handler->time_last_command_sent + step_command_handler->debounce;
      int time_left = last_command_finish_time - now;
      hal_tasks_schedule(&step_command_handler->update_task, time_left);

      step_command_handler->is_command_scheduled = true;
    }

    // Increase the next step amount
    step_command_handler->scheduled_change += step_command_handler->step_ammount;
  }

}
