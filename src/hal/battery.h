#ifndef _HAL_BATTERY_H_
#define _HAL_BATTERY_H_

#include "hal/gpio.h"
#include <stdint.h>

// Min and max battery voltages in mV for percentage calculation
// Applies to common coin cells: CR2032, CR2450, CR2430
#define BATTERY_VOLTAGE_MIN_MV    2000
#define BATTERY_VOLTAGE_MAX_MV    3000

/**
 * Initialize battery ADC with the given pin.
 * Must be called before hal_battery_get_voltage_mv().
 */
void hal_battery_init(hal_gpio_pin_t pin);

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

#endif // _HAL_BATTERY_H_
