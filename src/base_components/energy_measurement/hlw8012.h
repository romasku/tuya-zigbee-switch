#ifndef _HLW8012_H_
#define _HLW8012_H_

#include <stdint.h>
#include "hal/gpio.h"
#include "base_components/energy_meter.h"
#include "hal/tasks.h"

// Default calibration for TS011F-BS-PM-2 (BSEED), from hardware measurement
// against a precision multimeter and a purely resistive heating element
// (power factor 1, so true power = U * I). Reference point ~237 V / 3.10 A
// => 734.7 W:
//   237 V    <-> 0.0236 V/pulse  (voltage mode, CF1)
//   3.10 A   <-> 1.810 mA/pulse  (current mode, CF1)
//   734.7 W  <-> 0.2127 W/pulse  (power, CF)
// Physical value = pulses * MULTIPLIER / FIXED_POINT_SCALE.
// Output units: voltage in centivolts (0.01 V), power in W, current in mA.
// Different hardware revisions (same chip, different sense resistors/dividers)
// can override these via -D build flags; see device_db.yaml's
// hlw8012_voltage_multiplier/hlw8012_current_multiplier/hlw8012_power_multiplier.
#define HLW8012_FIXED_POINT_SCALE            65536
#ifndef HLW8012_POWER_MULTIPLIER
#define HLW8012_POWER_MULTIPLIER             13939  // 0.2127 W per pulse
#endif
#ifndef HLW8012_VOLTAGE_MULTIPLIER
#define HLW8012_VOLTAGE_MULTIPLIER           154672 // ~2.36 cV (0.0236 V) per pulse
#endif
#ifndef HLW8012_CURRENT_MULTIPLIER
#define HLW8012_CURRENT_MULTIPLIER           118646 // ~1.810 mA per pulse
#endif
#define HLW8012_SEL_TOGGLE_CYCLE_INTERVAL    5
#define HLW8012_PULSE_TIMEOUT_MS             20000
#define HLW8012_SAMPLE_INTERVAL_MS           5000
// Plausibility cap per sample (16A/230V ~= 6300 CF pulses/5s); above = glitch.
#define HLW8012_MAX_SANE_PULSES              30000
// Exact-canary calibrated no-load envelope: observed residual was 1 W / 37 mA.
#define HLW8012_NO_LOAD_POWER_W              2
#define HLW8012_NO_LOAD_CURRENT_MA           50
#define HLW8012_NO_LOAD_CONFIRM_SAMPLES      3

// Energy accumulates pulses*POWER_MULTIPLIER per sample; this many sub-units
// equal 1 Wh: FIXED_POINT_SCALE * 3600s / sample_seconds. Kept under 2^32 so
// energy can be derived with 32-bit subtraction (TC32 has no 64-bit divide).
#define HLW8012_ENERGY_WH_SUBUNIT \
        (HLW8012_FIXED_POINT_SCALE * 3600u / (HLW8012_SAMPLE_INTERVAL_MS / 1000u))

typedef struct {
    uint32_t cf_pulse_count;
    uint32_t cf_last_pulse_time;
    uint32_t cf_total_pulse_count;
    uint32_t cf1_pulse_count;
    uint32_t cf1_last_pulse_time;
    uint32_t cf1_total_pulse_count;
    uint32_t last_sample_time;
    uint32_t cf_tick_pulse_count;
    uint32_t cf1_tick_pulse_count;
    uint8_t  cf_last_gpio_state;
    uint8_t  cf1_last_gpio_state;
    uint16_t voltage;
    uint16_t current;
    int16_t  power;      // W
    uint32_t energy;
    uint32_t energy_acc; // energy sub-unit remainder (pulses*MULT, < 1 Wh)
    // Consecutive low samples prevent the calibrated no-load pulse floor from
    // becoming phantom power/energy or an overload input.
    uint8_t  no_load_samples;
    uint8_t  no_load_suppressed;
    uint8_t  sel_state;
    uint8_t  valid;
    uint32_t freq_cf;
    uint32_t freq_cf1;
    // Raw pulse counts that produced the most recent valid reading of each
    // channel, kept so on-device calibration can derive a multiplier directly
    // (multiplier = reference * FIXED_POINT_SCALE / pulses).
    uint32_t cal_pulses_voltage;
    uint32_t cal_pulses_current;
    uint32_t cal_pulses_power;
} hlw8012_data_t;

// Runtime calibration. Seeded from the compile-time defaults in hlw8012_init,
// then optionally overridden per device (e.g. from the config_str) so two
// boards with the same firmware but different sense resistors/dividers can be
// calibrated independently without a rebuild.
typedef struct {
    uint32_t voltage_multiplier;
    uint32_t current_multiplier;
    uint32_t power_multiplier;
} hlw8012_calibration_t;

typedef struct {
    hal_gpio_pin_t        cf_pin;
    hal_gpio_pin_t        cf1_pin;
    hal_gpio_pin_t        sel_pin;
    hal_gpio_counter_t    cf_counter;
    hal_gpio_counter_t    cf1_counter;
    hlw8012_data_t        data;
    hlw8012_calibration_t cal;
    hal_task_t            update_task;
    uint8_t               cycle_count;
    uint8_t               initialized;
    // The SEL pin selects which quantity CF1 carries. The HLW8012 and the
    // BL0937 use opposite polarity for that choice, so a board fitted with the
    // other chip reads voltage and current swapped. Set this to 1 to flip the
    // mapping (config_str `EP...I`).
    uint8_t               sel_inverted;
    energy_meter_t        meter;
} hlw8012_t;

int            hlw8012_init(hlw8012_t *dev, hal_gpio_pin_t cf_pin,
                            hal_gpio_pin_t cf1_pin, hal_gpio_pin_t sel_pin);

// Flip the SEL-pin meaning (HLW8012 vs BL0937). Call right after init.
void           hlw8012_set_sel_inverted(hlw8012_t *dev, uint8_t inverted);

// Override calibration multipliers. A zero argument keeps the current value,
// so callers can set only the multipliers they have a reference for.
void            hlw8012_set_calibration(hlw8012_t *dev, uint32_t voltage_mult,
                                        uint32_t current_mult, uint32_t power_mult);

// Calibrate one channel (0=voltage cV, 1=current mA, 2=power W) so it reads
// `reference` for the most recent raw pulse count. Returns 0 on success, -1 if
// there is no signal yet (pulse count 0) or the channel/reference is invalid.
int             hlw8012_calibrate(hlw8012_t *dev, uint8_t channel,
                                  uint32_t reference);
hlw8012_data_t *hlw8012_get_data(hlw8012_t *dev);
void            hlw8012_reset_energy(hlw8012_t *dev);
energy_meter_t *hlw8012_as_energy_meter(hlw8012_t *dev);
void            hlw8012_tick(hlw8012_t *dev);

#endif /* _HLW8012_H_ */
