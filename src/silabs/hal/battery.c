#include "hal/battery.h"

#ifdef BATTERY_POWERED

void hal_battery_reinit_after_retention(void) {
    // Silabs: no-op (ADC doesn't need re-init after retention)
}

// TODO: Implement actual IADC reading for VDD measurement
// Reference: Silicon Labs IADC examples

uint16_t hal_battery_get_voltage_mv(void) {
    // Placeholder: return nominal 3.0V
    return 3000;
}

#endif // BATTERY_POWERED
