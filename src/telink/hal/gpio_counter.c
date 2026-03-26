#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)

#include "hal/gpio.h"
#include <stdint.h>

extern GPIO_PullTypeDef hal_to_telink_pull(hal_gpio_pull_t pull);

// Maximum number of hardware counters available (TIMER0 and TIMER1)
#define MAX_GPIO_COUNTERS    2

typedef struct {
    hal_gpio_pin_t gpio_pin;
    uint8_t        timer_idx;
    uint8_t        in_use;
} gpio_counter_state_t;

static gpio_counter_state_t counter_state[MAX_GPIO_COUNTERS];
static uint8_t counters_initialized = 0;

static void init_counter_state(void) {
    if (!counters_initialized) {
        for (int i = 0; i < MAX_GPIO_COUNTERS; i++) {
            counter_state[i].gpio_pin  = HAL_INVALID_PIN;
            counter_state[i].timer_idx = i; // TIMER0=0, TIMER1=1
            counter_state[i].in_use    = 0;
        }
        counters_initialized = 1;
    }
}

hal_gpio_counter_t hal_gpio_counter_init(hal_gpio_pin_t gpio_pin,
                                         hal_gpio_counter_edge_t edge,
                                         hal_gpio_pull_t pull) {
    init_counter_state();

    // Find a free counter slot
    gpio_counter_state_t *slot   = NULL;
    hal_gpio_counter_t    handle = HAL_GPIO_COUNTER_INVALID;

    for (int i = 0; i < MAX_GPIO_COUNTERS; i++) {
        if (!counter_state[i].in_use) {
            slot   = &counter_state[i];
            handle = i;
            break;
        }
    }

    if (slot == NULL) {
        return HAL_GPIO_COUNTER_INVALID;
    }

    // Map HAL edge to Telink polarity
    GPIO_PolTypeDef pol        = (edge == HAL_GPIO_COUNTER_RISING) ? POL_RISING : POL_FALLING;
    GPIO_PinTypeDef telink_pin = (GPIO_PinTypeDef)gpio_pin;

    // Configure GPIO for timer trigger mode
    timer_gpio_init(slot->timer_idx, telink_pin, pol);

    // Enable GPIO interrupt for the selected timer
    if (slot->timer_idx == 0) {
        drv_gpio_irq_risc0_en(telink_pin);
    } else if (slot->timer_idx == 1) {
        drv_gpio_irq_risc1_en(telink_pin);
    }

    // Set pull resistor
    gpio_setup_up_down_resistor(telink_pin, hal_to_telink_pull(pull));

    // Set timer to GPIO trigger (counter) mode
    timer_set_mode(slot->timer_idx, TIMER_MODE_GPIO_TRIGGER);

    // Reset counter to 0
    timer_set_init_tick(slot->timer_idx, 0);

    // Set max count value
    timer_set_cap_tick(slot->timer_idx, 0xFFFFFFFF);

    // Enable timer interrupt (needed for counter operation)
    timer_irq_enable(slot->timer_idx);

    // Start the timer/counter
    timer_start(slot->timer_idx);

    // Mark slot as in use
    slot->gpio_pin = gpio_pin;
    slot->in_use   = 1;

    return handle;
}

void hal_gpio_counter_deinit(hal_gpio_counter_t counter) {
    if (counter < 0 || counter >= MAX_GPIO_COUNTERS) {
        return;
    }

    gpio_counter_state_t *slot = &counter_state[counter];
    if (!slot->in_use) {
        return;
    }

    // Stop the timer/counter
    hal_gpio_counter_stop(counter);

    // Reset the counter
    hal_gpio_counter_reset(counter);

    // Disable GPIO interrupt for the selected timer
    GPIO_PinTypeDef telink_pin = (GPIO_PinTypeDef)slot->gpio_pin;
    if (slot->timer_idx == 0) {
        drv_gpio_irq_risc0_dis(telink_pin);
    } else if (slot->timer_idx == 1) {
        drv_gpio_irq_risc1_dis(telink_pin);
    }

    // Reset slot state
    slot->gpio_pin = HAL_INVALID_PIN;
    slot->in_use   = 0;
}

uint32_t hal_gpio_counter_read(hal_gpio_counter_t counter) {
    if (counter < 0 || counter >= MAX_GPIO_COUNTERS) {
        return 0;
    }

    gpio_counter_state_t *slot = &counter_state[counter];
    if (!slot->in_use) {
        return 0;
    }

    return reg_tmr_tick(slot->timer_idx);
}

void hal_gpio_counter_reset(hal_gpio_counter_t counter) {
    if (counter < 0 || counter >= MAX_GPIO_COUNTERS) {
        return;
    }

    gpio_counter_state_t *slot = &counter_state[counter];
    if (!slot->in_use) {
        return;
    }

    timer_set_init_tick(slot->timer_idx, 0);
}

void hal_gpio_counter_start(hal_gpio_counter_t counter) {
    if (counter < 0 || counter >= MAX_GPIO_COUNTERS) {
        return;
    }

    gpio_counter_state_t *slot = &counter_state[counter];
    if (!slot->in_use) {
        return;
    }

    timer_start(slot->timer_idx);
}

void hal_gpio_counter_stop(hal_gpio_counter_t counter) {
    if (counter < 0 || counter >= MAX_GPIO_COUNTERS) {
        return;
    }

    gpio_counter_state_t *slot = &counter_state[counter];
    if (!slot->in_use) {
        return;
    }

    timer_stop(slot->timer_idx);
}
