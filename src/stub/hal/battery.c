#include "hal/battery.h"

#ifdef BATTERY_POWERED

// Stub implementation for testing
static uint8_t  stub_battery_percentage = 100;
static uint16_t stub_battery_voltage_mv = 3000;

uint8_t hal_battery_get_percentage(void) {
    return stub_battery_percentage;
}

uint16_t hal_battery_get_voltage_mv(void) {
    return stub_battery_voltage_mv;
}

void hal_battery_reinit_after_retention(void) {
    // Stub: no-op
}

// Test helper function (called from Python tests via ctypes)
void stub_set_battery_percentage(uint8_t percentage) {
    stub_battery_percentage = (percentage > 100) ? 100 : percentage;
}

#endif // BATTERY_POWERED
