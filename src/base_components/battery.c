#include "battery.h"
#include "hal/adc.h"

void battery_init(battery_t *battery) {
    hal_adc_init(HAL_ADC_INPUT_VBAT, battery->pin);
    if (battery->charge_range == 0) {
        battery->charge_range = 200;
    }
}

battery_status_t battery_get_status(battery_t *battery) {
    uint16_t         voltage_mv = hal_adc_read_mv();
    battery_status_t status     = {
        .voltage_mv = voltage_mv
    };

    if (voltage_mv < battery->voltage_min) {
        status.charge = 0;
    } else if (voltage_mv > battery->voltage_max) {
        status.charge = battery->charge_range;
    } else {
        uint32_t voltage_range  = battery->voltage_max - battery->voltage_min;
        uint32_t voltage_offset = voltage_mv - battery->voltage_min;
        status.charge = voltage_offset * battery->charge_range / voltage_range;
    }
    return status;
}
