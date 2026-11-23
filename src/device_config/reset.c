#include "reset.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/system.h"
#include "hal/tasks.h"
#include <stdint.h>

static hal_task_t reset_task;

__attribute__((noreturn)) void reset_all() {
    printf("RESET ALL!\r\n");
    hal_nvm_clear_all();
    hal_factory_reset();
    hal_system_reset();
}

void reset_all_handler(void *arg) {
    reset_all();
}

void reboot_handler(void *arg) {
    hal_system_reset();
}

void schedule_full_reset(uint16_t delay_ms) {
    reset_task.handler = reset_all_handler;
    hal_tasks_init(&reset_task);
    hal_tasks_schedule(&reset_task,
                       delay_ms != 0 ? delay_ms : DEFAULT_RESET_DELAY_MS);
}

void schedule_reboot(uint16_t delay_ms) {
    reset_task.handler = reboot_handler;
    hal_tasks_init(&reset_task);
    hal_tasks_schedule(&reset_task,
                       delay_ms != 0 ? delay_ms : DEFAULT_RESET_DELAY_MS);
}
