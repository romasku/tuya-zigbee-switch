#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "hal/gpio.h"
#include <stdio.h>


hal_gpio_counter_t hal_gpio_counter_init(hal_gpio_pin_t gpio_pin,
                                         hal_gpio_counter_edge_t edge,
                                         hal_gpio_pull_t pull) {
    // No-op for now

    return HAL_GPIO_COUNTER_INVALID;
}

void hal_gpio_counter_deinit(hal_gpio_counter_t counter) {
    // No-op for now
}

uint32_t hal_gpio_counter_read(hal_gpio_counter_t counter) {
    return 0;
}

void hal_gpio_counter_reset(hal_gpio_counter_t counter) {
    // No-op for now
}

void hal_gpio_counter_start(hal_gpio_counter_t counter) {
    // No-op for now
}

void hal_gpio_counter_stop(hal_gpio_counter_t counter) {
    // No-op for now
}
