#include <stdint.h>
#include <stdlib.h>
#include "step_command_handler.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include "hal/printf_selector.h"
#include "consts.h"

static void _register_callback(step_command_handler_2_t*self,  step_command_handler_callback_t callback, void * callback_arg);
static void _step_up(step_command_handler_2_t *self);
static void _step_down(step_command_handler_2_t *self);
static void _trigger_callback(void *arg);

// Constructor
step_command_handler_2_t * new_step_command_handler() {

  step_command_handler_2_t *step_command_handler = malloc(sizeof(step_command_handler_2_t));

  step_command_handler->_scheduled_change = 0;

  step_command_handler->step_up = _step_up;
  step_command_handler->step_down = _step_down;
  step_command_handler->register_callback = _register_callback;

  return step_command_handler;
}

// Public Functions
static void _step_up(step_command_handler_2_t *self) {
  self->_scheduled_change += 10;

  _trigger_callback(self);
}

static void _step_down(step_command_handler_2_t *self) {
  self->_scheduled_change -= 10;

  _trigger_callback(self);
}

static void _register_callback(step_command_handler_2_t*self,  step_command_handler_callback_t callback, void * callback_arg) {
  self->_callback = callback;
  self->_callback_arg = callback_arg;
}

// Private Functions
static void _trigger_callback(void *arg) {
  step_command_handler_2_t *step_command_handler = (step_command_handler_2_t *)arg;

  printf("Sending command, to change by %d in %d*0.1sec\r\n", step_command_handler->_scheduled_change, 1);

  step_command_handler->_callback(step_command_handler->_callback_arg, step_command_handler->_scheduled_change, 1);
}

