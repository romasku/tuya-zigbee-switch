#ifndef _HAL_BATTERY_H_
#define _HAL_BATTERY_H_

#include <stdint.h>

#ifdef BATTERY_POWERED

// Min and max battery voltages in mV for percentage calculation
// Applies to common coin cells: CR2032, CR2450, CR2430
#define BATTERY_VOLTAGE_MIN_MV    2000
#define BATTERY_VOLTAGE_MAX_MV    3000

/**
 * Get current battery voltage (millivolts)
 * @return Battery voltage in mV, or 0 if unavailable
 */
uint16_t hal_battery_get_voltage_mv(void);

/**
 * Reinitialize battery ADC after deep sleep retention wakeup
 * Platform-specific: only implemented for platforms that need it
 */
void hal_battery_reinit_after_retention(void);

#endif // BATTERY_POWERED

#endif // _HAL_BATTERY_H_
