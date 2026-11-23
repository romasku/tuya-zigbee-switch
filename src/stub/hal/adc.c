#include "hal/adc.h"

static uint16_t stub_adc_voltage_mv = 3000;

void hal_adc_init(hal_adc_input_t input, hal_gpio_pin_t pin) {
    (void)input;
    (void)pin;
}

uint16_t hal_adc_read_mv() {
    return stub_adc_voltage_mv;
}

void stub_set_adc_voltage_mv(uint16_t voltage_mv) {
    stub_adc_voltage_mv = voltage_mv;
}

void stub_set_battery_voltage_mv(uint16_t voltage_mv) {
    stub_set_adc_voltage_mv(voltage_mv);
}
