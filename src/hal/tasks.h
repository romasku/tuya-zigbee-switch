#ifndef _HAL_TASKS_H_
#define _HAL_TASKS_H_

#include <stdint.h>

/** Function called when a scheduled task executes */
typedef void (*task_handler_t)(void *arg);

#ifdef HAL_SILABS

#include "zigbee_app_framework_event.h"

typedef sli_zigbee_event_t hal_platfrom_struct_t;

#endif

#ifdef HAL_TELINK

typedef struct {
  void *ev_timer_handle; // Can't use telink header due to size_t conflict
} hal_platfrom_struct_t;

#endif

#ifdef HAL_STUB

typedef struct {
  void *dummy; // Placeholder for stub implementation
} hal_platfrom_struct_t;

#endif

/** Schedulable task for delayed execution (timers, debouncing, periodic
 * actions) */
typedef struct {
  task_handler_t handler;
  void *arg;
  hal_platfrom_struct_t platform_struct;
} hal_task_t;

/**
 * Initialize a task for use with the scheduler
 * @param task Task structure to initialize
 */
void hal_tasks_init(hal_task_t *task);

/**
 * Schedule a task to execute after a delay
 * @param task Task to schedule
 * @param delay_ms Delay in milliseconds before execution
 */
void hal_tasks_schedule(hal_task_t *task, uint32_t delay_ms);

/**
 * Cancel a previously scheduled task
 * @param task Task to cancel
 */
void hal_tasks_unschedule(hal_task_t *task);

#endif /* HAL_TASKS_H_ */
