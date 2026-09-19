#ifndef _ELECTRICAL_MEASUREMENT_CLUSTER_H_
#define _ELECTRICAL_MEASUREMENT_CLUSTER_H_

#include <stdint.h>
#include "hal/zigbee.h"
#include "base_components/energy_meter.h"
#include "base_components/overload_protection.h"

typedef struct {
    uint8_t               endpoint;
    energy_meter_t *      meter;
    uint32_t              measurement_type;
    uint16_t              rms_voltage;
    uint16_t              rms_current;
    int16_t               active_power;
    // Derived (computed from V, I, P — the BL0942/HLW give no phase, so these
    // are the true/total quantities): apparent power S = Vrms*Irms in VA,
    // reactive power Q = sqrt(S^2 - P^2) magnitude in var, and power factor
    // P/S as a signed percentage (equals cos φ only for linear loads).
    uint16_t              apparent_power;
    int16_t               reactive_power;
    int8_t                power_factor;
    uint16_t              ac_voltage_multiplier;
    uint16_t              ac_voltage_divisor;
    uint16_t              ac_current_multiplier;
    uint16_t              ac_current_divisor;
    uint16_t              ac_power_multiplier;
    uint16_t              ac_power_divisor;
    uint32_t              freq_cf;
    uint32_t              freq_cf1;
    uint8_t               sel_state;
    // On-device calibration inputs: write the real measured value (voltage in
    // cV, current in mA, power in W) to calibrate that channel. Reset to 0 by
    // the firmware once applied.
    uint16_t              calibrate_voltage;
    uint16_t              calibrate_current;
    uint16_t              calibrate_power;
    // Read/write ZCL string mirroring the active calibration multipliers as
    // "V<mult>A<mult>W<mult>". Reading it lets a calibrated reference device's
    // values be copied to other devices of the same type; writing applies and
    // persists the multipliers (a missing or 0 channel keeps its value).
    struct {
        uint8_t len;
        char    str[36]; // fits V+A+W with 10-digit uint32 multipliers
    }                     calibration_values;
    // Overload protection: live state machine, the relay it guards (opaque
    // zigbee_relay_cluster*, set by the config parser), and the ZCL-facing
    // config/alarm mirror attributes.
    overload_protection_t overload;
    void *                protected_relay;
    uint16_t              overload_power_limit;
    uint16_t              overload_current_limit;
    uint16_t              overload_trip_delay;
    uint16_t              overvoltage_warn;
    uint16_t              undervoltage_warn;
    uint16_t              overload_reconnect_delay;
    uint8_t               overload_alarm;
    hal_zigbee_attribute  attr_infos[27];
    uint32_t              last_report_time;
    uint16_t              last_reported_voltage;
    uint16_t              last_reported_current;
    int16_t               last_reported_power;
} electrical_measurement_cluster_t;

// Derive apparent power (VA), reactive power (var, magnitude) and power factor
// (signed %) from the RMS voltage (cV), RMS current (mA) and active power (W).
// Pure integer math (no float, no phase input), factored out so it is unit-
// testable. Any output pointer may be NULL.
void elec_meas_derive_power(uint16_t voltage_cv, uint16_t current_ma,
                            int16_t active_power_w, uint16_t *out_apparent_va,
                            int16_t *out_reactive_var, int8_t *out_power_factor);

void electrical_measurement_cluster_init(electrical_measurement_cluster_t *cluster,
                                         energy_meter_t *meter);
void electrical_measurement_cluster_add_to_endpoint(
    electrical_measurement_cluster_t *cluster, hal_zigbee_endpoint *endpoint);

// Enable overload protection by giving the meter cluster the relay it guards
// (a zigbee_relay_cluster*, passed opaquely to avoid an include cycle).
void electrical_measurement_cluster_set_protected_relay(
    electrical_measurement_cluster_t *cluster, void *relay_cluster);
void electrical_measurement_cluster_update(electrical_measurement_cluster_t *cluster);
void electrical_measurement_cluster_report(electrical_measurement_cluster_t *cluster);

// Load persisted calibration from NVM and apply it to the meter (call after
// the meter is initialised, on boot).
void electrical_measurement_cluster_load_calibration(
    electrical_measurement_cluster_t *cluster);

// Handle writes to the calibrate_* attributes (compute + persist calibration).
void electrical_measurement_cluster_callback_attr_write_trampoline(
    uint8_t endpoint, uint16_t attribute_id);

#endif /* _ELECTRICAL_MEASUREMENT_CLUSTER_H_ */
