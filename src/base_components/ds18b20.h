#ifndef _DS18B20_H_
#define _DS18B20_H_

#include "onewire_bus.h"

#define DS18B20_FAMILY    0x28

/* Match + Read Scratchpad. Fills the temperature fields when the CRC matches. */
void ds18b20_read(const uint8_t rom[8], ow_reading_t *reading,
                  int16_t *temp_c_x100);

#endif