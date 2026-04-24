#include <stdint.h>
#include <stdlib.h>
#include "step_command_handler.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include "hal/printf_selector.h"
#include "consts.h"

static void register_callback(step_command_handler_2_t*self,  step_command_handler_callback_t callback, void * callback_arg);
static void step_up(step_command_handler_2_t *self);
static void step_down(step_command_handler_2_t *self);
static void _trigger_callback(step_command_handler_2_t *self);

// Constructor
step_command_handler_2_t * new_step_command_handler() {

  step_command_handler_2_t *self = malloc(sizeof(step_command_handler_2_t));

  self->_scheduled_change = 0;
  self->_last_command_sent_time = 0;
  self->_callback_running = false;

  self->step_up = step_up;
  self->step_down = step_down;
  self->register_callback = register_callback;

  return self;
}

// Public Functions
static void step_up(step_command_handler_2_t *self) {
  self->_scheduled_change += 10;

  printf("_last_command_sent_time is %d \r\n", self->_last_command_sent_time);

  if(!self->_callback_running && self->_last_command_sent_time + 50 < hal_millis()) {
    // Last command was sent over 50 millis ago, we can send another
    _trigger_callback(self);
  }
}

static void step_down(step_command_handler_2_t *self) {
  self->_scheduled_change -= 10;

  if(!self->_callback_running && self->_last_command_sent_time + 50 < hal_millis()) {
    // Last command was sent over 50 millis ago, we can send another
    _trigger_callback(self);
  }
}

static void register_callback(step_command_handler_2_t*self,  step_command_handler_callback_t callback, void * callback_arg) {
  self->_callback = callback;
  self->_callback_arg = callback_arg;
}

// Private Functions
static void _trigger_callback(step_command_handler_2_t *self) {
  self->_callback_running = true; // Guard against more calls coming in while this one is completing

  printf("Sending command, to change by %d in %d*0.1sec\r\n", self->_scheduled_change, 1);
  
  self->_callback(self->_callback_arg, self->_scheduled_change, 1);

  self->_last_command_sent_time = hal_millis();
  self->_scheduled_change = 0;
  
  self->_callback_running = false;
}

