#ifndef _BATTERY_H_
#define _BATTERY_H_

#include "hal/gpio.h"

typedef struct {
    hal_gpio_pin_t pin;
    uint16_t       voltage_min;
    uint16_t       voltage_max;
    uint16_t       charge_range;
} battery_t;

typedef struct {
    uint16_t voltage_mv;
    uint16_t charge;
} battery_status_t;

void battery_init(battery_t *battery);

battery_status_t battery_get_status(battery_t *battery);

#endif // _BATTERY_H_
