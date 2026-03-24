#ifndef _BATTERY_H_
#define _BATTERY_H_

#include "hal/gpio.h"

typedef struct {
    hal_gpio_pin_t pin;
    uint16_t       voltage_min;
    uint16_t       voltage_max;
} battery_t;

void battery_init(battery_t *battery);

uint16_t battery_get_mv(battery_t *battery);

uint16_t battery_get_charge(battery_t *battery, uint16_t range);

#endif // _BATTERY_H_
