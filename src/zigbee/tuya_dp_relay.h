#ifndef _TUYA_DP_RELAY_H_
#define _TUYA_DP_RELAY_H_

#include <stdint.h>

/* One datapoint pushed to the secondary MCU at startup (IT token). */
typedef struct {
    uint8_t dpid;
    uint8_t dp_type;
    uint8_t len;
    uint8_t value[4];
} dp_init_entry_t;

/*
 * Bridges relays whose output lives behind a Tuya secondary MCU (TS0601-class
 * devices) to the standard ZCL OnOff path.
 *
 * Outgoing: relay_on()/relay_off() -> DP write over UART.
 * Incoming: DP report (physical press or echo) -> relay state + ZCL report.
 *
 * Must be called AFTER tuya_secondary_mcu_init() and after the config string
 * has been parsed.
 */
void tuya_dp_relay_init(void);

/* Write every IT-declared datapoint to the secondary MCU. */
void tuya_dp_apply_inits(void);

/* Number of parsed relays that are DP-backed rather than GPIO-backed. */
uint8_t tuya_dp_relay_count(void);

#endif
