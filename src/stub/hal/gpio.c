#include "hal/gpio.h"
#include "stub/machine_io.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_GPIO_PINS    256
#define MAX_CALLBACKS    32

typedef struct {
    uint8_t         initialized;
    uint8_t         is_input;
    uint8_t         value;
    hal_gpio_pull_t pull;
    gpio_callback_t callback;
    void *          callback_arg;
} stub_gpio_pin_t;

static stub_gpio_pin_t gpio_pins[MAX_GPIO_PINS];

void ensure_valid_input_pin(hal_gpio_pin_t gpio_pin);
void ensure_valid_output_pin(hal_gpio_pin_t gpio_pin);

void hal_gpio_init(hal_gpio_pin_t gpio_pin, uint8_t is_input,
                   hal_gpio_pull_t pull) {
    if (gpio_pin >= MAX_GPIO_PINS)
        return;

    gpio_pins[gpio_pin].initialized  = 1;
    gpio_pins[gpio_pin].is_input     = is_input;
    gpio_pins[gpio_pin].pull         = pull;
    gpio_pins[gpio_pin].value        = (pull == HAL_GPIO_PULL_UP) ? 1 : 0;
    gpio_pins[gpio_pin].callback     = NULL;
    gpio_pins[gpio_pin].callback_arg = NULL;

    io_log("GPIO", "Init pin %d as %s, pull=%d", gpio_pin,
           is_input ? "input" : "output", pull);
}

void hal_gpio_set(hal_gpio_pin_t gpio_pin) {
    ensure_valid_output_pin(gpio_pin);

    gpio_pins[gpio_pin].value = 1;
    io_log("GPIO", "Set pin %d = 1", gpio_pin);
    io_evt("gpio pin=%d value=%d", gpio_pin, 1);
}

void hal_gpio_clear(hal_gpio_pin_t gpio_pin) {
    ensure_valid_output_pin(gpio_pin);

    gpio_pins[gpio_pin].value = 0;
    io_log("GPIO", "Clear pin %d = 0", gpio_pin);
    io_evt("gpio pin=%d value=%d", gpio_pin, 0);
}

uint8_t hal_gpio_read(hal_gpio_pin_t gpio_pin) {
    ensure_valid_input_pin(gpio_pin);

    io_log("GPIO", "Read pin %d = %d", gpio_pin, gpio_pins[gpio_pin].value);
    return gpio_pins[gpio_pin].value;
}

void hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback,
                       void *arg) {
    ensure_valid_input_pin(gpio_pin);

    gpio_pins[gpio_pin].callback     = callback;
    gpio_pins[gpio_pin].callback_arg = arg;
    io_log("GPIO", "Set callback for pin %d", gpio_pin);
}

void hal_gpio_unreg_callback(hal_gpio_pin_t gpio_pin) {
    ensure_valid_output_pin(gpio_pin);

    gpio_pins[gpio_pin].callback     = NULL;
    gpio_pins[gpio_pin].callback_arg = NULL;
    io_log("GPIO", "Unregistered callback for pin %d", gpio_pin);
}

hal_gpio_pin_t hal_gpio_parse_pin(const char *s) {
    if (!s) {
        io_log("GPIO", "Error: NULL string passed to hal_parse_gpio_pin");
        return HAL_INVALID_PIN;
    }

    // Simple parsing: expect format like "PA5" or "PB10"
    if (strlen(s) < 2) {
        io_log("GPIO", "Error: Invalid GPIO pin format: '%s'", s);
        return HAL_INVALID_PIN;
    }

    char port = s[0];
    int  pin  = atoi(&s[1]);

    if (port < 'A' || port > 'Z' || pin < 0 || pin > 15) {
        io_log("GPIO", "Error: Invalid GPIO pin format: '%s'", s);
        return HAL_INVALID_PIN;
    }

    hal_gpio_pin_t res = ((port - 'A') << 4) | pin;
    io_log("GPIO", "Parsed GPIO pin '%s' as %d", s, res);
    return res;
}

hal_gpio_pull_t hal_gpio_parse_pull(const char *pull_str) {
    if (!pull_str) {
        io_log("GPIO", "Error: NULL string passed to hal_parse_gpio_pull");
        return HAL_GPIO_PULL_INVALID;
    }

    if (strcmp(pull_str, "") == 0)
        return HAL_GPIO_PULL_NONE;

    if (strcmp(pull_str, "u") == 0)
        return HAL_GPIO_PULL_UP;

    if (strcmp(pull_str, "d") == 0)
        return HAL_GPIO_PULL_DOWN;

    io_log("GPIO", "Error: Invalid GPIO pull string: '%s'", pull_str);
    return HAL_GPIO_PULL_INVALID;
}

void hal_gpio_reinit_all(void) {
    // No-op in stub (no SFR to restore)
}

void hal_gpio_reinit_interrupts(void) {
    // No-op in stub (no SFR to restore)
}

// Stub-specific functions for testing

void stub_gpio_simulate_input(hal_gpio_pin_t gpio_pin, uint8_t value) {
    if (gpio_pin >= MAX_GPIO_PINS) {
        io_log("GPIO", "Error: Invalid input pin %d for simulation", gpio_pin);
        return;
    }

    uint8_t old_value = gpio_pins[gpio_pin].value;
    gpio_pins[gpio_pin].value = value;

    if (old_value != value && gpio_pins[gpio_pin].callback) {
        gpio_pins[gpio_pin].callback(gpio_pin, gpio_pins[gpio_pin].callback_arg);
    }

    io_log("GPIO", "Simulated input pin %d = %d", gpio_pin, value);
}

uint8_t stub_gpio_get_output(hal_gpio_pin_t gpio_pin) {
    if (gpio_pin >= MAX_GPIO_PINS) {
        io_log("GPIO", "Error: Invalid GPIO pin %d for output", gpio_pin);
        return 0;
    }
    return gpio_pins[gpio_pin].value;
}

// Helper funcs

void ensure_valid_pin(hal_gpio_pin_t gpio_pin) {
    if (gpio_pin >= MAX_GPIO_PINS) {
        io_log("GPIO", "Error: GPIO pin %d out of range", gpio_pin);
        exit(1);
    }
    if (!gpio_pins[gpio_pin].initialized) {
        io_log("GPIO", "Error: GPIO pin %d not initialized", gpio_pin);
        exit(1);
    }
}

void ensure_valid_input_pin(hal_gpio_pin_t gpio_pin) {
    ensure_valid_pin(gpio_pin);
    if (!gpio_pins[gpio_pin].is_input) {
        io_log("GPIO", "Error: Attempt to use output GPIO pin %d as input",
               gpio_pin);
        exit(1);
    }
}

void ensure_valid_output_pin(hal_gpio_pin_t gpio_pin) {
    ensure_valid_pin(gpio_pin);
    if (gpio_pins[gpio_pin].is_input) {
        io_log("GPIO", "Error: Attempt to use input GPIO pin %d as output",
               gpio_pin);
        exit(1);
    }
}
