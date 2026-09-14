#ifndef _METERING_BL0937_H_
#define _METERING_BL0937_H_

#include <stdint.h>

#include "hal/gpio.h"
#include "hal/tasks.h"

/*
 * BL0937 / HLW8012-family energy metering driver.
 *
 * The chip outputs pulses on two pins:
 *   - CF:  pulse frequency proportional to active power. Pulses are also
 *          accumulated for energy metering.
 *   - CF1: pulse frequency proportional to either RMS current or RMS
 *          voltage, selected with the SEL pin.
 *
 * The driver alternates SEL every `interval_ms`, reading the hardware
 * pulse counters (see hal_gpio_counter_* API) at each phase boundary.
 * The first CF1 interval after each SEL toggle belongs to the newly
 * selected magnitude, so no sample is discarded; frequency is computed
 * from the pulse count over the elapsed interval.
 *
 * Calibration: coefficients express pulse frequency in mHz per output
 * unit. Defaults approximate a typical BL0937 plug (1 mOhm shunt,
 * ~1:2000 voltage divider) and MUST be trimmed per board. See
 * `docs/contribute/` notes in the PR for the calibration procedure.
 */

typedef void (*metering_callback_t)(void *param);

typedef struct {
    // Wiring (set before metering_bl0937_init)
    hal_gpio_pin_t cf_pin;
    hal_gpio_pin_t cf1_pin;
    hal_gpio_pin_t sel_pin;
    uint8_t        sel_current_level; // SEL level that routes CURRENT to CF1

    // Behaviour
    uint32_t interval_ms;             // per-phase sampling window (default 2000)

    // Calibration (pulse frequency in mHz per engineering unit)
    uint32_t coef_power_mhz_per_dw;   // mHz per deciwatt
    uint32_t coef_voltage_mhz_per_dv; // mHz per decivolt
    uint32_t coef_current_mhz_per_ma; // mHz per milliamp
    uint32_t pulses_per_wh;           // CF pulses per accumulated Wh

    // Outputs (engineering units, ZCL friendly)
    uint16_t voltage_dv;              // RMS voltage, 0.1 V
    uint16_t current_ma;              // RMS current, mA
    int16_t  power_dw;                // active power, 0.1 W
    uint64_t energy_pulses;           // lifetime CF pulses (energy source)
    uint32_t energy_wh;               // derived accumulated energy, Wh

    // Notification
    metering_callback_t on_update;    // fired after each phase completes
    void *              callback_param;

    // Internal state
    hal_gpio_counter_t cf_counter;
    hal_gpio_counter_t cf1_counter;
    uint8_t            sel_state;     // 1 while CF1 carries current
    uint32_t           residual_pulses; // CF remainder for Wh conversion
    uint32_t           last_sample_ms;
    hal_task_t         update_task;
} metering_bl0937_t;

/**
 * Initialize the metering driver. Pins and (optionally) calibration must
 * be filled in beforehand; zeroed calibration fields get defaults.
 * @return 0 on success, -1 if hardware counters are unavailable
 */
int8_t metering_bl0937_init(metering_bl0937_t *m);

/** Stop sampling and release the hardware counters */
void metering_bl0937_deinit(metering_bl0937_t *m);

#endif
