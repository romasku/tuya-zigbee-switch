#include <stdint.h>
#include <stdlib.h>
#include "step_command_handler.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include "hal/printf_selector.h"
#include "consts.h"

void step_command_handle_step_up(step_command_handler_2_t *self) {
  printf("Step Up Called, value is %d\r\n", self->value);
}

step_command_handler_2_t * new_step_command_handler() {

  step_command_handler_2_t *step_command_handler = malloc(sizeof(step_command_handler_2_t));

  step_command_handler->step_up = step_command_handle_step_up;

  return step_command_handler;
}

void send_command(void *arg) {
  step_command_handler_t *step_command_handler = (step_command_handler_t *)arg;
  printf("Sending command, to change by %d in %d ms\r\n", step_command_handler->scheduled_change, step_command_handler->debounce);

  step_command_handler->sent_zigbee_command(step_command_handler->sent_zigbee_command_param, 0, step_command_handler->scheduled_change, step_command_handler->debounce);

  step_command_handler->time_last_command_sent = hal_millis();
  step_command_handler->is_command_scheduled = false;
  step_command_handler->scheduled_change = 0;
}

void setup_step_command_handler(step_command_handler_t *step_command_handler) {
  step_command_handler->debounce = 100; // ms
  step_command_handler->step_ammount = 10;

  printf("setting time last sent to %d\r\n", hal_millis());
  step_command_handler->time_last_command_sent = hal_millis();
  step_command_handler->is_command_scheduled = false;
  step_command_handler->scheduled_change = 0;

  step_command_handler->update_task.handler = send_command;
  step_command_handler->update_task.arg     = step_command_handler;
  hal_tasks_init(&step_command_handler->update_task);

  printf("setting time last sent to %d\r\n", step_command_handler->time_last_command_sent);
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
      printf("scheduling a task in %d\r\n", time_left);
      hal_tasks_schedule(&step_command_handler->update_task, time_left);

      step_command_handler->is_command_scheduled = true;
    }

    // Increase the next step amount
    step_command_handler->scheduled_change += step_command_handler->step_ammount;
  }

}
