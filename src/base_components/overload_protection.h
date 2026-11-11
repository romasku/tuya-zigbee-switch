#ifndef _OVERLOAD_PROTECTION_H_
#define _OVERLOAD_PROTECTION_H_

#include <stdint.h>

// Two-tier overload protection for a mains relay driven by an energy meter.
//
// Tier 1 (soft, configurable): if power or current stays above the soft limit
// for longer than trip_delay, the relay is switched off.
// Tier 2 (hard, fixed manufacturer peak): power/current above the hard limit
// trips the relay immediately, no delay.
// Voltage is warn-only (never trips) — it only raises the alarm status.
//
// After a trip the re-enable behaviour follows the relay's power-on (StartUp
// OnOff) setting: OFF stays off until switched on manually; ON/PREVIOUS auto-
// reconnect after reconnect_delay, up to OVERLOAD_MAX_RETRIES times, then latch
// off with a distinct "locked out" alarm. Turning the relay on manually clears
// the latch and re-arms.
//
// This module is pure logic (no zigbee / no hardware deps): the caller feeds
// measurements + the relay state and applies the returned action, so it is
// unit-testable and reused by both meter drivers.

// Default manufacturer peak limits (A1Z and BSEED PM are both 16 A / 3680 W).
// These are only the compile-time defaults: the actual per-device hard caps
// live in overload_config_t and can be raised via the config_str `OL` token
// (e.g. the TONGOU breaker is rated 16 A continuous / 20 A peak).
#define OVERLOAD_HARD_POWER_W       3680u
#define OVERLOAD_HARD_CURRENT_MA    16000u
#define OVERLOAD_MAX_RETRIES        5u

// Nominal mains voltage used to derive a power limit from a current limit
// (P = I * V). The user configures the device by its rated current; the
// matching wattage is computed at this reference voltage.
#define OVERLOAD_NOMINAL_VOLTAGE_V    230u

// Undervoltage below this (in cV) is treated as "no reading" (unplugged /
// meter not up yet), not a brownout, so it does not raise a false alarm.
#define OVERLOAD_VOLTAGE_FLOOR_CV    5000u // 50 V

typedef enum {
    OVERLOAD_ALARM_NONE         = 0,
    OVERLOAD_ALARM_POWER        = 1, // over the soft power limit
    OVERLOAD_ALARM_CURRENT      = 2, // over the soft current limit
    OVERLOAD_ALARM_PEAK         = 3, // over the hard (peak) limit
    OVERLOAD_ALARM_VOLTAGE_HIGH = 4, // overvoltage warning (no trip)
    OVERLOAD_ALARM_VOLTAGE_LOW  = 5, // undervoltage warning (no trip)
    OVERLOAD_ALARM_LOCKED_OUT   = 6, // tripped repeatedly, latched off
} overload_alarm_t;

typedef enum {
    OVERLOAD_ACTION_NONE     = 0,
    OVERLOAD_ACTION_TURN_OFF = 1,
    OVERLOAD_ACTION_TURN_ON  = 2,
} overload_action_t;

typedef struct {
    uint16_t power_limit_w;     // soft power limit (W); 0 disables this check
    uint16_t current_limit_ma;  // soft current limit (mA); 0 disables
    uint16_t trip_delay_s;      // grace time above soft limit before tripping
    uint16_t overvoltage_cv;    // overvoltage warn threshold (cV); 0 disables
    uint16_t undervoltage_cv;   // undervoltage warn threshold (cV); 0 disables
    uint16_t reconnect_delay_s; // auto-reconnect delay after a trip
    // Hard (peak) caps that trip instantly. These are the device's rating, not
    // user-configurable from Z2M — they come from the firmware defaults or the
    // config_str `OL` token and are never overwritten by persisted NVM.
    uint16_t hard_power_w;      // peak power cap (W); 0 disables the power check
    uint16_t hard_current_ma;   // peak current cap (mA); 0 disables
} overload_config_t;

typedef struct {
    overload_config_t cfg;
    overload_alarm_t  alarm;
    uint8_t           tripped;         // relay was forced off by us
    uint8_t           locked_out;      // gave up after MAX_RETRIES
    uint8_t           retry_count;
    uint32_t          over_since_ms;   // first tick over soft limit (0 = not)
    uint32_t          reconnect_at_ms; // scheduled auto-reconnect (0 = none)
} overload_protection_t;

// Initialize with the factory-default thresholds.
void overload_protection_init(overload_protection_t *op);

// Set the soft (continuous) and hard (peak) current caps in mA and derive the
// matching power caps at OVERLOAD_NOMINAL_VOLTAGE_V. A 0 argument leaves that
// tier unchanged. Used by the config parser to apply a device's rated limits
// from the config_str `OL` token before NVM is loaded.
void overload_protection_set_current_limits(overload_protection_t *op,
                                            uint16_t soft_current_ma,
                                            uint16_t hard_current_ma);

// Feed fresh measurements (voltage cV, current mA, power W — power should be
// the fastest available estimate) plus the relay's current on-state and its
// StartUpOnOff mode. Returns the action to apply to the relay. `now_ms` is a
// monotonic millisecond clock.
overload_action_t overload_protection_check(overload_protection_t *op,
                                            uint32_t now_ms, uint16_t voltage_cv,
                                            uint16_t current_ma, int32_t power_w,
                                            uint8_t relay_is_on,
                                            uint8_t startup_mode);

#endif /* _OVERLOAD_PROTECTION_H_ */
