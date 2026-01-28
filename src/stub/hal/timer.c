#include "hal/timer.h"
#include "stub/machine_io.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static uint64_t start_ms;
static int      initialized   = 0;
static int      time_frozen   = 0;
static uint64_t frozen_millis = 0;

void stub_millis_init() {
    struct timeval current_time;

    if (gettimeofday(&current_time, NULL) != 0) {
        io_log("TIMER", "Error: Failed to get initial time");
        exit(1);
    }
    start_ms    = (uint64_t)current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
    initialized = 1;
    io_log("TIMER", "Timer initialized with start time %ld.%06ld",
           current_time.tv_sec, current_time.tv_usec);
}

void stub_millis_freeze() {
    frozen_millis = hal_millis();
    time_frozen   = 1;
    io_log("TIMER", "Time frozen at %llu ms", (unsigned long long)frozen_millis);
}

void stub_millis_unfreeze() {
    time_frozen = 0;
    io_log("TIMER", "Time unfrozen");
}

void stub_millis_step(uint64_t step) {
    frozen_millis += step;
}

uint32_t hal_millis() {
    if (time_frozen) {
        return (uint32_t)frozen_millis;
    }

    struct timeval current_time;

    if (!initialized) {
        stub_millis_init();
    }

    if (gettimeofday(&current_time, NULL) != 0) {
        io_log("TIMER", "Error: Failed to get current time");
        exit(1);
    }

    uint64_t current_ms =
        (uint64_t)current_time.tv_sec * 1000 + current_time.tv_usec / 1000;

    uint32_t result = (uint32_t)(current_ms - start_ms);

    return result;
}
