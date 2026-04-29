#include <stdint.h>
#include <stdlib.h>
#include "step_command_handler.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include "hal/printf_selector.h"
#include "consts.h"

static void _trigger_callback(step_command_handler_t *self);

// Constructor
void new_step_command_handler(step_command_handler_t *self) {

  self->_scheduled_change = 0;
  self->_last_command_sent_time = 0;
}

// Public Functions
void step_command_handler_step_up(step_command_handler_t *self) {
  self->_scheduled_change += 13;

  uint32_t now = hal_millis();

  // If current millis is less then the last command send time, then millis has rolled over.
  if(self->_last_command_sent_time > now) {
    self->_last_command_sent_time = now - 100;
  }

  if(self->_last_command_sent_time + 100 <= now) {
    // Last command was sent over 50 millis ago, we can send another
    _trigger_callback(self);
  }
}

void step_command_handler_step_down(step_command_handler_t *self) {
  self->_scheduled_change -= 13;

  uint32_t now = hal_millis();

  // If current millis is less then the last command send time, then millis has rolled over.
  if(self->_last_command_sent_time > now) {
    self->_last_command_sent_time = now - 100;
  }

  if(self->_last_command_sent_time + 100 <= now) {
    // Last command was sent over 50 millis ago, we can send another
    _trigger_callback(self);
  }
}

void step_command_handler_register_callback(step_command_handler_t*self,  step_command_handler_callback_t callback, void * callback_arg) {
  self->_callback = callback;
  self->_callback_arg = callback_arg;
}

// Private Functions
static void _trigger_callback(step_command_handler_t *self) {
  printf("Sending command, to change by %d in %d*0.1sec\r\n", self->_scheduled_change, 1);
  
  self->_callback(self->_callback_arg, self->_scheduled_change, 1);

  self->_last_command_sent_time = hal_millis();
  self->_scheduled_change = 0;
}

