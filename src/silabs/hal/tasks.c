#include "hal/tasks.h"

static void _af_event_handler(sli_zigbee_event_t *event) {
  hal_task_t *task = (hal_task_t *)event->data;
  task->handler(task->arg);
}

void hal_tasks_init(hal_task_t *task) {
  sl_zigbee_af_event_init(&task->platform_struct, _af_event_handler);
  task->platform_struct.data = (uint32_t)task;
}

void hal_tasks_schedule(hal_task_t *task, uint32_t delay_ms) {
  sl_zigbee_af_event_set_delay_ms(&task->platform_struct, delay_ms);
}

void hal_tasks_unschedule(hal_task_t *task) {
  sl_zigbee_af_event_set_inactive(&task->platform_struct);
}
