#include "hal/gpio.h"
#include "atoi.h"
#include "hal/tasks.h"
#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)

#include <stdint.h>
#include <string.h>
// hal_gpio_pin_t directly stores GPIO_PinTypeDef values

// GPIO config tracking for deep retention re-init
// After deep retention wake, all GPIO SFRs are lost and must be replayed.
#define MAX_GPIO_CONFIGS    16


typedef struct {
    hal_gpio_pin_t  pin;
    uint8_t         is_input;
    hal_gpio_pull_t pull;
    uint8_t         value;
} gpio_config_t;
static gpio_config_t gpio_configs[MAX_GPIO_CONFIGS];
static uint8_t       gpio_config_cnt = 0;

static gpio_config_t *find_gpio_config(hal_gpio_pin_t pin) {
    for (uint8_t i = 0; i < gpio_config_cnt; i++) {
        if (gpio_configs[i].pin == pin) {
            return &gpio_configs[i];
        }
    }
    return NULL;
}

// Convert HAL pull type to Telink pull type
static GPIO_PullTypeDef hal_to_telink_pull(hal_gpio_pull_t pull) {
    switch (pull) {
    case HAL_GPIO_PULL_NONE:
        return PM_PIN_UP_DOWN_FLOAT;

    case HAL_GPIO_PULL_UP:
        return PM_PIN_PULLUP_10K;

    case HAL_GPIO_PULL_UP_1M:
        return PM_PIN_PULLUP_1M;

    case HAL_GPIO_PULL_DOWN:
        return PM_PIN_PULLDOWN_100K;

    default:
        return PM_PIN_UP_DOWN_FLOAT;
    }
}

static void gpio_init_hw(hal_gpio_pin_t gpio_pin, uint8_t is_input,
                         hal_gpio_pull_t pull) {
    GPIO_PinTypeDef telink_pin = (GPIO_PinTypeDef)gpio_pin;

    if (is_input) {
        gpio_set_input_en(telink_pin, 1);
        gpio_set_output_en(telink_pin, 0);
    } else {
        gpio_set_output_en(telink_pin, 1);
        gpio_set_input_en(telink_pin, 0);
    }

    // Set pull resistor
    gpio_setup_up_down_resistor(telink_pin, hal_to_telink_pull(pull));
}

void hal_gpio_init(hal_gpio_pin_t gpio_pin, uint8_t is_input,
                   hal_gpio_pull_t pull) {
    gpio_init_hw(gpio_pin, is_input, pull);

    gpio_config_t *config = find_gpio_config(gpio_pin);
    if (config == NULL) {
        if (gpio_config_cnt >= MAX_GPIO_CONFIGS) {
            return;
        }
        config = &gpio_configs[gpio_config_cnt++];
    }

    // Track config for deep retention re-init
    config->pin      = gpio_pin;
    config->is_input = is_input;
    config->value    = 0;
    config->pull     = pull;
}

void telink_gpio_reinit_after_deep_retention(void) {
    for (uint8_t i = 0; i < gpio_config_cnt; i++) {
        gpio_init_hw(gpio_configs[i].pin, gpio_configs[i].is_input,
                     gpio_configs[i].pull);
        if (!gpio_configs[i].is_input) {
            // Restore output value
            gpio_write((GPIO_PinTypeDef)gpio_configs[i].pin, gpio_configs[i].value);
        }
    }
}

void telink_gpio_to_pull_for_deep_retention() {
    // Uses pull-up or downs to emulate output levels during deep retention,
    // as telink doesn't support true GPIO output retention.
    for (uint8_t i = 0; i < gpio_config_cnt; i++) {
        if (gpio_configs[i].is_input) {
            continue;
        }
        GPIO_PinTypeDef  telink_pin = (GPIO_PinTypeDef)gpio_configs[i].pin;
        GPIO_PullTypeDef pull       =
            gpio_configs[i].value ? PM_PIN_PULLUP_1M : PM_PIN_PULLDOWN_100K;
        gpio_set_input_en(telink_pin, 1);
        gpio_set_output_en(telink_pin, 0);
        gpio_setup_up_down_resistor(telink_pin, pull);
    }
}

void hal_gpio_set(hal_gpio_pin_t gpio_pin) {
    gpio_write((GPIO_PinTypeDef)gpio_pin, 1);
    gpio_config_t *config = find_gpio_config(gpio_pin);
    if (config) {
        config->value = 1;
    }
}

void hal_gpio_clear(hal_gpio_pin_t gpio_pin) {
    gpio_write((GPIO_PinTypeDef)gpio_pin, 0);
    gpio_config_t *config = find_gpio_config(gpio_pin);
    if (config) {
        config->value = 0;
    }
}

uint8_t hal_gpio_read(hal_gpio_pin_t gpio_pin) {
    return gpio_read((GPIO_PinTypeDef)gpio_pin) ? 1 : 0;
}

hal_gpio_pin_t hal_gpio_parse_pin(const char *s) {
    if (!s || strlen(s) < 2) {
        return HAL_INVALID_PIN;
    }

    // Parse format like "A5", "B3", etc.
    char port = s[0];
    int  pin  = atoi(&s[1]);

    if (pin < 0 || pin > 7) {
        return HAL_INVALID_PIN;
    }

    // Return direct Telink GPIO_PinTypeDef values
    uint16_t port_base;
    switch (port) {
    case 'A':
    case 'a':
        port_base = GPIO_GROUPA;
        break;
    case 'B':
    case 'b':
        port_base = GPIO_GROUPB;
        break;
    case 'C':
    case 'c':
        port_base = GPIO_GROUPC;
        break;
    case 'D':
    case 'd':
        port_base = GPIO_GROUPD;
        break;
    case 'E':
    case 'e':
        port_base = GPIO_GROUPE;
        break;
    default:
        return HAL_INVALID_PIN;
    }

    return (hal_gpio_pin_t)(port_base | BIT(pin));
}

hal_gpio_pull_t hal_gpio_parse_pull(const char *pull_str) {
    if (!pull_str) {
        return HAL_GPIO_PULL_INVALID;
    }

    switch (pull_str[0]) {
    case 'u':
        return HAL_GPIO_PULL_UP;

    case 'U':
        return HAL_GPIO_PULL_UP_1M;

    case 'd':
    case 'D':
        return HAL_GPIO_PULL_DOWN;

    case 'f':
    case 'F':
    case 'n':
    case 'N':
        return HAL_GPIO_PULL_NONE;

    default:
        return HAL_GPIO_PULL_INVALID;
    }
}

// GPIO interrupt callbacks
