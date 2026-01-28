#include "hal/tasks.h"
#include "hal/timer.h"
#include "stub/machine_io.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_TASKS    32

typedef struct {
    hal_task_t *task;
    uint32_t    scheduled_time;
    int         active;
} stub_task_entry_t;

static stub_task_entry_t tasks[MAX_TASKS];

void stub_tasks_poll(void) {
    uint32_t current_time   = hal_millis();
    int      tasks_executed = 0;

    // Check all tasks for execution, keep looping until no tasks are executed in
    // a pass, to more agressively tests for tasks that reschedule themselves.
    do {
        tasks_executed = 0;
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].active && current_time >= tasks[i].scheduled_time) {
                if (tasks[i].task && tasks[i].task->handler) {
                    io_log("TASKS", "Executing task %p from slot %d",
                           (void *)tasks[i].task, i);
                    tasks[i].task->handler(tasks[i].task->arg);
                    tasks_executed++;
                }
                tasks[i].active = 0;
                io_log("TASKS", "Task completed and removed from slot %d", i);
            }
        }
    } while (tasks_executed > 0);
}

void hal_tasks_init(hal_task_t *task) {
    if (!task) {
        io_log("TASKS", "Error: NULL task pointer passed to hal_init_task");
        exit(1);
    }

    memset(&task->platform_struct, 0, sizeof(task->platform_struct));
    io_log("TASKS", "Initialized task at %p", (void *)task);
}

void hal_tasks_schedule(hal_task_t *task, uint32_t delay_ms) {
    if (!task) {
        io_log("TASKS", "Error: NULL task pointer passed to hal_schedule_task");
        exit(1);
    }

    if (!task->handler) {
        io_log("TASKS", "Error: Task at %p has NULL handler", (void *)task);
        exit(1);
    }

    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        io_log("TASKS", "Error: No free task slots available (max %d tasks)",
               MAX_TASKS);
        exit(1);
    }

    tasks[slot].task           = task;
    tasks[slot].scheduled_time = hal_millis() + delay_ms;
    tasks[slot].active         = 1;

    io_log("TASKS", "Scheduled task %p in slot %d, delay=%u ms, execute_at=%u",
           (void *)task, slot, delay_ms, tasks[slot].scheduled_time);
}

void hal_tasks_unschedule(hal_task_t *task) {
    if (!task)
        return;

    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].active && tasks[i].task == task) {
            tasks[i].active = 0;
            io_log("TASKS", "Unscheduled task %p from slot %d", (void *)task, i);
            break;
        }
    }
}
