#ifndef _ONEWIRE_H_
#define _ONEWIRE_H_

#include "hal/gpio.h"
#include <stdint.h>

#define OW_MAX_SENSORS    8

/*
 * One 1-Wire bus, up to OW_MAX_SENSORS devices in any mix.
 * Family 0x28 (DS18B20) fills the temperature value of its slot.
 * Family 0x26 (DS2438) fills the humidity value of its slot.
 * The bus is searched once per boot, until no further device answers.
 * A ROM already in NV keeps its slot. A new ROM takes a free slot, or, when
 * all 8 are occupied, a slot whose saved sensor did not answer this search.
 */
void ow_init(hal_gpio_pin_t pin);
uint8_t ow_is_configured(void);

/*
 * Search the bus one device per call so the main loop can clear the watchdog
 * between steps. An empty bus finishes in a single step.
 * ow_search_step returns 1 while another step is required.
 */
void ow_search_begin(void);
uint8_t ow_search_step(void);
/* Second boot pass for a slave that ignores the 600 µs reset. */
void ow_search_alternate_resets(void);

/* Bit i is set when slot i answered the search that just finished. */
uint8_t ow_seen_mask(void);
/* ROM family of a seen slot, or 0. 0x28 is DS18B20, 0x26 is DS2438. */
uint8_t ow_slot_family(uint8_t slot);

/*
 * Match each seen sensor and send Convert T. Skip ROM is ignored when one
 * slave is emulating more than one device.
 * Returns 1 when the caller should wait before reading.
 */
uint8_t ow_convert_start(void);
uint8_t ow_sensors_seen(void);

/*
 * Read one sensor per call. Output slots start invalid; a slot that did not
 * answer stays invalid. Returns 1 while another sensor still has to be read.
 */
void ow_read_begin(int16_t temps_c_x100[OW_MAX_SENSORS],
                   uint16_t rh_percent_x100[OW_MAX_SENSORS]);
uint8_t ow_read_step(int16_t temps_c_x100[OW_MAX_SENSORS],
                     uint16_t rh_percent_x100[OW_MAX_SENSORS]);

/* One debug line: ROM, slot origin, endpoint, and the last reading. */
void ow_log_slot(uint8_t slot, uint8_t endpoint);

#endif
