#ifndef _DS2438_H_
#define _DS2438_H_

#include "onewire_bus.h"

#define DS2438_FAMILY    0x26

/* VAD and VDD from page 0, then the HobbyBoards humidity formula. */
void ds2438_read(const uint8_t rom[8], ow_reading_t *reading,
                 uint16_t *rh_percent_x100);

#endif