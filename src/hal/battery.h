#ifndef _HAL_BATTERY_H_
#define _HAL_BATTERY_H_

#include <stdint.h>

#ifdef BATTERY_POWERED

/**
 * Get current battery percentage (0-100%)
 * @return Battery level 0-100 percent
 */
uint8_t hal_battery_get_percentage(void);

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
