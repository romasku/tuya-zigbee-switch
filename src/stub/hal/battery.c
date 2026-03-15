#include "hal/battery.h"

#ifdef BATTERY_POWERED

// Stub implementation for testing
static uint16_t stub_battery_voltage_mv = 3000;

uint16_t hal_battery_get_voltage_mv(void) {
    return stub_battery_voltage_mv;
}

void hal_battery_reinit_after_retention(void) {
    // Stub: no-op
}

// Test helper function (called from Python tests via ctypes)
void stub_set_battery_voltage_mv(uint16_t voltage_mv) {
    stub_battery_voltage_mv = voltage_mv;
}

#endif // BATTERY_POWERED
