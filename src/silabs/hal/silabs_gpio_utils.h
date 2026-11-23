#pragma once

#include <stdint.h>

#include "em_gpio.h"
#include "sl_gpio.h"

#include "hal/gpio.h"

// Silabs HAL encoding: upper byte = GPIO_Port_TypeDef, lower byte = pin [0..15].
static inline uint8_t silabs_hal_gpio_pin_number(hal_gpio_pin_t pin) {
    return (uint8_t)(pin & 0xFF);
}

static inline GPIO_Port_TypeDef silabs_hal_gpio_port(hal_gpio_pin_t pin) {
    return (GPIO_Port_TypeDef)((pin >> 8) & 0xFF);
}

static inline hal_gpio_pin_t silabs_hal_gpio_make_pin(GPIO_Port_TypeDef port,
                                                      uint8_t pin_no) {
    return (hal_gpio_pin_t)(((uint16_t)port << 8) | pin_no);
}

static inline sl_gpio_t silabs_hal_gpio_to_sl_gpio(hal_gpio_pin_t pin) {
    return (sl_gpio_t){
               .port = silabs_hal_gpio_port(pin),
               .pin  = silabs_hal_gpio_pin_number(pin),
    };
}
