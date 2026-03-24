#include "battery.h"
#include "hal/adc.h"

void battery_init(battery_t *battery) {
    hal_adc_init(HAL_ADC_INPUT_VBAT, battery->pin);
}

uint16_t battery_get_mv(battery_t *battery) {
    return hal_adc_read_mv();
}

uint16_t battery_get_charge(battery_t *battery, uint16_t range) {
    uint16_t voltage_mv = battery_get_mv(battery);

    if (voltage_mv < battery->voltage_min) {
        return 0;
    } else if (voltage_mv > battery->voltage_max) {
        return range;
    } else {
        return (uint32_t)(voltage_mv - battery->voltage_min) * range /
               (battery->voltage_max - battery->voltage_min);
    }
}
