#ifndef RESET_H
#define RESET_H

#include <stdint.h>

#define DEFAULT_RESET_DELAY_MS 300

__attribute__((noreturn)) void reset_all();

void schedule_full_reset(uint16_t delay_ms);
void schedule_reboot(uint16_t delay_ms);

#endif // RESET_H