#ifndef _TUYA_MCU_APP_H_
#define _TUYA_MCU_APP_H_

#include <stdbool.h>
#include <stdint.h>

#include "hal/zigbee.h"

/*
 * Application side of the secondary MCU: starts the UART link and its users
 * (DP relays, dp attributes), answers the MCU's network/time queries, and
 * makes first contact (IT values + full DP dump) once the MCU listens.
 */

/** True if the configuration puts anything behind a secondary MCU (dimmers,
 *  DP relays/buttons/IT values, dp attributes). */
bool tuya_mcu_app_in_use(void);

/** Call after parse_config(); does nothing on devices without an MCU. */
void tuya_mcu_app_init(void);

/** Call from the main loop; does nothing on devices without an MCU. */
void tuya_mcu_app_task(void);

/** Append the MCU's attributes (firmware version) to a Basic cluster table;
 *  returns how many. */
uint8_t tuya_mcu_app_register_attrs(hal_zigbee_attribute *table, uint8_t start_index);

/** ZCL StartUpOnOff <-> the common Tuya power-on DP (0 off, 1 on, 2 memory).
 *  "Toggle" has no Tuya equivalent and maps to memory. */
uint8_t tuya_mcu_power_on_from_startup(uint8_t startup_mode);
uint8_t tuya_mcu_startup_from_power_on(uint8_t power_on);

#endif
