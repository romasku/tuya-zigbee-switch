#ifndef _DS18B20_H_
#define _DS18B20_H_

#include "hal/gpio.h"
#include <stdint.h>

#define DS18B20_MAX_SENSORS    8

/*
 * One 1-Wire bus, up to DS18B20_MAX_SENSORS devices in any mix.
 * Family 0x28 (DS18B20) fills the temperature value of its slot.
 * Family 0x26 (DS2438) fills the humidity value of its slot.
 * Slot assignment is stored in NV. A slot that misses three scans is released.
 */
void ds18b20_init(hal_gpio_pin_t pin);
uint8_t ds18b20_is_configured(void);

/* Search the bus, keep slot assignment, start a conversion if any sensor answered. */
void ds18b20_start_conversion(void);

/*
 * Read the sensors that answered the last scan.
 * temps_c_x100[i] is 0.01 °C, or ZCL_TEMPERATURE_INVALID when the slot is empty
 * or the sensor did not answer.
 * Returns the number of slots that produced a reading.
 */
uint8_t ds18b20_read_temperatures(int16_t temps_c_x100[DS18B20_MAX_SENSORS]);

/*
 * Read every DS2438 that answered the last scan.
 * rh_percent_x100[i] is 0.01 %RH, or 0xFFFF when the slot is empty,
 * is a DS18B20, or the sensor did not answer.
 * Returns the number of slots that produced a humidity reading.
 */
uint8_t ds18b20_read_humidities(uint16_t rh_percent_x100[DS18B20_MAX_SENSORS]);

#endif
