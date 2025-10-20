#include "hal/tasks.h"
#include "tl_common.h"

// Wrapper callback to adapt between Telink's int return signature and HAL's
// void signature
static int _telink_task_wrapper(void *data) {
  hal_task_t *task = (hal_task_t *)data;

  // Clear the timer handle since it's now executed
  task->platform_struct.ev_timer_handle = NULL;

  // Call the HAL task handler
  task->handler(task->arg);

  // Return -1 to indicate no rescheduling
  return -1;
}

void hal_tasks_init(hal_task_t *task) {
  // Initialize the platform struct
  task->platform_struct.ev_timer_handle = NULL;
}

void hal_tasks_schedule(hal_task_t *task, uint32_t delay_ms) {
  // Cancel any existing scheduled task
  if (task->platform_struct.ev_timer_handle != NULL) {
    ev_timer_taskCancel(
        (ev_timer_event_t **)&task->platform_struct.ev_timer_handle);
  }

  // Schedule new task using Telink's event timer system
  task->platform_struct.ev_timer_handle =
      ev_timer_taskPost(_telink_task_wrapper, // Wrapper callback
                        task,                 // Pass the hal_task_t as argument
                        delay_ms              // Delay in milliseconds
      );
}

void hal_tasks_unschedule(hal_task_t *task) {
  // Cancel the scheduled task if it exists
  if (task->platform_struct.ev_timer_handle != NULL) {
    ev_timer_taskCancel(
        (ev_timer_event_t **)&task->platform_struct.ev_timer_handle);
    task->platform_struct.ev_timer_handle = NULL;
  }
}