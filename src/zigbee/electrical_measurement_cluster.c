#include "electrical_measurement_cluster.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/timer.h"
#include "hal/printf_selector.h"
#include "relay_cluster.h"
#include <string.h>

#define MEAS_TYPE_AC_ACTIVE      (1 << 0)
#define MEAS_TYPE_AC_REACTIVE    (1 << 1)
#define MEAS_TYPE_AC_APPARENT    (1 << 2)
#define MEAS_TYPE_PHASE_A        (1 << 3)

// Bit-by-bit integer square root (no float / libm on the target M0/M33).
static uint32_t isqrt_u32(uint32_t x) {
    uint32_t res = 0;
    uint32_t bit = 1u << 30;

    while (bit > x)
        bit >>= 2;
    while (bit != 0) {
        if (x >= res + bit) {
            x  -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

void elec_meas_derive_power(uint16_t voltage_cv, uint16_t current_ma,
                            int16_t active_power_w, uint16_t *out_apparent_va,
                            int16_t *out_reactive_var, int8_t *out_power_factor) {
    // Apparent power S = Vrms * Irms. Voltage is in cV, current in mA, so
    // S[VA] = voltage_cv * current_ma / 100000. Clamp to the uint16 attribute
    // range (comfortably above any single-socket mains load).
    uint32_t s_va = (uint32_t)voltage_cv * (uint32_t)current_ma / 100000u;

    if (s_va > 0xFFFFu)
        s_va = 0xFFFFu;

    int32_t p     = active_power_w;
    int32_t p_abs = p < 0 ? -p : p;

    // Power factor = P / S as a signed percentage, clamped to [-100, 100]
    // (calibration/rounding can make |P| edge slightly past S).
    int8_t pf = 0;
    if (s_va > 0) {
        int32_t pf32 = p * 100 / (int32_t)s_va;
        if (pf32 > 100)
            pf32 = 100;
        else if (pf32 < -100)
            pf32 = -100;
        pf = (int8_t)pf32;
    }

    // Reactive power Q = sqrt(S^2 - P^2), magnitude only — the lead/lag sign is
    // not observable from RMS magnitudes without a phase reference. Clamp the
    // radicand at 0 so meter noise (|P| just over S) never yields a bogus root.
    uint32_t q_var = 0;
    uint32_t s2    = s_va * s_va;
    uint32_t p2    = (uint32_t)(p_abs * p_abs);
    if (s2 > p2)
        q_var = isqrt_u32(s2 - p2);
    if (q_var > 0x7FFFu)
        q_var = 0x7FFFu;

    if (out_apparent_va)
        *out_apparent_va = (uint16_t)s_va;
    if (out_reactive_var)
        *out_reactive_var = (int16_t)q_var;
    if (out_power_factor)
        *out_power_factor = pf;
}

// Marks a valid persisted-calibration record.
#define ELEC_MEAS_CAL_NV_MAGIC    0x484C5743u // "HLWC"

typedef struct {
    uint32_t magic;
    uint32_t voltage_multiplier;
    uint32_t current_multiplier;
    uint32_t power_multiplier;
} elec_meas_cal_nv_t;

// Single instance, used to route attribute-write callbacks to calibration.
static electrical_measurement_cluster_t *g_elec_cluster = NULL;

static void elec_meas_run_overload_protection(
    electrical_measurement_cluster_t *cluster, const energy_meter_data_t *data);

static void elec_meas_save_calibration(electrical_measurement_cluster_t *cluster) {
    energy_meter_calibration_t cal;

    energy_meter_get_calibration(cluster->meter, &cal);
    elec_meas_cal_nv_t nv = {
        .magic              = ELEC_MEAS_CAL_NV_MAGIC,
        .voltage_multiplier = cal.voltage_multiplier,
        .current_multiplier = cal.current_multiplier,
        .power_multiplier   = cal.power_multiplier,
    };
    hal_nvm_write(NV_ITEM_ENERGY_CALIBRATION, sizeof(nv), (uint8_t *)&nv);
}

// Append `value` as decimal digits at str+pos, returning the new position.
static uint8_t append_u32_dec(char *str, uint8_t pos, uint32_t value) {
    char    digits[10];
    uint8_t n = 0;

    do {
        digits[n++] = (char)('0' + (value % 10u));
        value      /= 10u;
    } while (value != 0);
    while (n > 0) {
        str[pos++] = digits[--n];
    }
    return pos;
}

// Rebuild the calibration_values string ("V<mult>A<mult>W<mult>") from the
// meter's active multipliers, so a read always returns the canonical form.
static void elec_meas_refresh_calibration_values(
    electrical_measurement_cluster_t *cluster) {
    energy_meter_calibration_t cal;
    uint8_t pos = 0;

    energy_meter_get_calibration(cluster->meter, &cal);
    cluster->calibration_values.str[pos++] = 'V';
    pos = append_u32_dec(cluster->calibration_values.str, pos,
                         cal.voltage_multiplier);
    cluster->calibration_values.str[pos++] = 'A';
    pos = append_u32_dec(cluster->calibration_values.str, pos,
                         cal.current_multiplier);
    cluster->calibration_values.str[pos++] = 'W';
    pos = append_u32_dec(cluster->calibration_values.str, pos,
                         cal.power_multiplier);
    cluster->calibration_values.len = pos;
}

// Parse "V<mult>A<mult>W<mult>" (any order, each channel optional; 0 or a
// missing channel keeps the current multiplier) and apply + persist it.
static void elec_meas_apply_calibration_values(
    electrical_measurement_cluster_t *cluster) {
    const char *str      = cluster->calibration_values.str;
    uint8_t     len      = cluster->calibration_values.len;
    uint32_t    mults[3] = { 0, 0, 0 }; // V, A, W

    if (len >= sizeof(cluster->calibration_values.str))
        len = sizeof(cluster->calibration_values.str) - 1;

    for (uint8_t i = 0; i < len; i++) {
        int8_t channel = -1;
        if (str[i] == 'V')
            channel = 0;
        else if (str[i] == 'A')
            channel = 1;
        else if (str[i] == 'W')
            channel = 2;

        if (channel < 0)
            continue;

        uint32_t value = 0;
        for (i++; i < len && str[i] >= '0' && str[i] <= '9'; i++) {
            value = value * 10u + (uint32_t)(str[i] - '0');
        }
        i--; // outer loop increments past the last digit
        mults[channel] = value;
    }

    energy_meter_set_calibration(cluster->meter, mults[0], mults[1], mults[2]);
    elec_meas_save_calibration(cluster);
}

// ---- Overload protection ----------------------------------------------------

// Keep the soft limits within the fixed manufacturer peak; a too-short
// reconnect delay is bumped up. The hard peak tier is always enforced by the
// state machine regardless of the soft settings, so protection can never be
// fully disabled.
static void elec_meas_overload_clamp(overload_config_t *c) {
    // The soft limit can never exceed the device's hard (peak) cap, so
    // protection can't be weakened past the rating. Fall back to the compiled
    // defaults if a cap is unset.
    uint16_t hp = c->hard_power_w ? c->hard_power_w : OVERLOAD_HARD_POWER_W;
    uint16_t hc = c->hard_current_ma ? c->hard_current_ma : OVERLOAD_HARD_CURRENT_MA;

    if (c->power_limit_w > hp)
        c->power_limit_w = hp;
    if (c->current_limit_ma > hc)
        c->current_limit_ma = hc;
    if (c->reconnect_delay_s < 5)
        c->reconnect_delay_s = 5;
}

static void elec_meas_overload_mirror_to_attrs(
    electrical_measurement_cluster_t *cluster) {
    cluster->overload_power_limit     = cluster->overload.cfg.power_limit_w;
    cluster->overload_current_limit   = cluster->overload.cfg.current_limit_ma;
    cluster->overload_trip_delay      = cluster->overload.cfg.trip_delay_s;
    cluster->overvoltage_warn         = cluster->overload.cfg.overvoltage_cv;
    cluster->undervoltage_warn        = cluster->overload.cfg.undervoltage_cv;
    cluster->overload_reconnect_delay = cluster->overload.cfg.reconnect_delay_s;
    cluster->overload_alarm           = (uint8_t)cluster->overload.alarm;
}

static void elec_meas_overload_save(electrical_measurement_cluster_t *cluster) {
    hal_nvm_write(NV_ITEM_OVERLOAD_CONFIG, sizeof(cluster->overload.cfg),
                  (uint8_t *)&cluster->overload.cfg);
}

static void elec_meas_overload_load(electrical_measurement_cluster_t *cluster) {
    overload_config_t nv;

    // Hard caps are the device's rating (from firmware defaults or the
    // config_str `OL` token, applied before this load). Never let a persisted
    // copy — which may predate a cap change — override them.
    uint16_t hard_power   = cluster->overload.cfg.hard_power_w;
    uint16_t hard_current = cluster->overload.cfg.hard_current_ma;

    if (hal_nvm_read(NV_ITEM_OVERLOAD_CONFIG, sizeof(nv), (uint8_t *)&nv) ==
        HAL_NVM_SUCCESS) {
        cluster->overload.cfg = nv; // keep compile-time defaults on first boot
    }
    cluster->overload.cfg.hard_power_w    = hard_power;
    cluster->overload.cfg.hard_current_ma = hard_current;
    elec_meas_overload_clamp(&cluster->overload.cfg);
}

// Pull the mirror attributes back into the config after a ZCL write, clamp,
// persist, and refresh the mirror to the canonical value.
static void elec_meas_overload_apply_attrs(
    electrical_measurement_cluster_t *cluster) {
    cluster->overload.cfg.power_limit_w     = cluster->overload_power_limit;
    cluster->overload.cfg.current_limit_ma  = cluster->overload_current_limit;
    cluster->overload.cfg.trip_delay_s      = cluster->overload_trip_delay;
    cluster->overload.cfg.overvoltage_cv    = cluster->overvoltage_warn;
    cluster->overload.cfg.undervoltage_cv   = cluster->undervoltage_warn;
    cluster->overload.cfg.reconnect_delay_s = cluster->overload_reconnect_delay;
    elec_meas_overload_clamp(&cluster->overload.cfg);
    elec_meas_overload_save(cluster);
    elec_meas_overload_mirror_to_attrs(cluster);
}

void electrical_measurement_cluster_set_protected_relay(
    electrical_measurement_cluster_t *cluster, void *relay_cluster) {
    if (cluster)
        cluster->protected_relay = relay_cluster;
}

void electrical_measurement_cluster_init(electrical_measurement_cluster_t *cluster,
                                         energy_meter_t *meter) {
    if (!cluster || !meter)
        return;

    memset(cluster, 0, sizeof(electrical_measurement_cluster_t));
    cluster->meter            = meter;
    cluster->measurement_type = MEAS_TYPE_AC_ACTIVE | MEAS_TYPE_AC_REACTIVE |
                                MEAS_TYPE_AC_APPARENT | MEAS_TYPE_PHASE_A;
    cluster->ac_voltage_multiplier = 1;
    cluster->ac_voltage_divisor    = 100;  // firmware reports voltage in centivolts
    cluster->ac_current_multiplier = 1;
    cluster->ac_current_divisor    = 1000; // firmware reports current in mA
    cluster->ac_power_multiplier   = 1;
    cluster->ac_power_divisor      = 1;    // firmware reports power in whole watts

    overload_protection_init(&cluster->overload);
    elec_meas_overload_mirror_to_attrs(cluster);
}

void electrical_measurement_cluster_add_to_endpoint(
    electrical_measurement_cluster_t *cluster, hal_zigbee_endpoint *endpoint) {
    if (!cluster || !endpoint)
        return;

    SETUP_ATTR(0, ZCL_ATTR_ELEC_MEAS_MEASUREMENT_TYPE, ZCL_DATA_TYPE_BITMAP32, ATTR_READONLY,
               cluster->measurement_type);
    SETUP_ATTR(1, ZCL_ATTR_ELEC_MEAS_RMS_VOLTAGE, ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->rms_voltage);
    SETUP_ATTR(2, ZCL_ATTR_ELEC_MEAS_RMS_CURRENT, ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->rms_current);
    SETUP_ATTR(3, ZCL_ATTR_ELEC_MEAS_ACTIVE_POWER, ZCL_DATA_TYPE_INT16, ATTR_READONLY,
               cluster->active_power);
    SETUP_ATTR(4, ZCL_ATTR_ELEC_MEAS_AC_VOLTAGE_MULTIPLIER, ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->ac_voltage_multiplier);
    SETUP_ATTR(5, ZCL_ATTR_ELEC_MEAS_AC_VOLTAGE_DIVISOR, ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->ac_voltage_divisor);
    SETUP_ATTR(6, ZCL_ATTR_ELEC_MEAS_AC_CURRENT_MULTIPLIER, ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->ac_current_multiplier);
    SETUP_ATTR(7, ZCL_ATTR_ELEC_MEAS_AC_CURRENT_DIVISOR, ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->ac_current_divisor);
    SETUP_ATTR(8, ZCL_ATTR_ELEC_MEAS_AC_POWER_MULTIPLIER, ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->ac_power_multiplier);
    SETUP_ATTR(9, ZCL_ATTR_ELEC_MEAS_AC_POWER_DIVISOR, ZCL_DATA_TYPE_UINT16, ATTR_READONLY,
               cluster->ac_power_divisor);
    SETUP_ATTR(10, ZCL_ATTR_ELEC_MEAS_CUST_FREQUENCY_CF, ZCL_DATA_TYPE_UINT32, ATTR_READONLY,
               cluster->freq_cf);
    SETUP_ATTR(11, ZCL_ATTR_ELEC_MEAS_CUST_FREQUENCY_CF1, ZCL_DATA_TYPE_UINT32, ATTR_READONLY,
               cluster->freq_cf1);
    SETUP_ATTR(12, ZCL_ATTR_ELEC_MEAS_CUST_FREQUENCY_SEL_STATE, ZCL_DATA_TYPE_UINT8, ATTR_READONLY,
               cluster->sel_state);
    SETUP_ATTR(13, ZCL_ATTR_ELEC_MEAS_CUST_CALIBRATE_VOLTAGE, ZCL_DATA_TYPE_UINT16, ATTR_WRITABLE,
               cluster->calibrate_voltage);
    SETUP_ATTR(14, ZCL_ATTR_ELEC_MEAS_CUST_CALIBRATE_CURRENT, ZCL_DATA_TYPE_UINT16, ATTR_WRITABLE,
               cluster->calibrate_current);
    SETUP_ATTR(15, ZCL_ATTR_ELEC_MEAS_CUST_CALIBRATE_POWER, ZCL_DATA_TYPE_UINT16, ATTR_WRITABLE,
               cluster->calibrate_power);
    SETUP_ATTR(16, ZCL_ATTR_ELEC_MEAS_CUST_CALIBRATION_VALUES, ZCL_DATA_TYPE_CHAR_STR,
               ATTR_WRITABLE,
               cluster->calibration_values);
    SETUP_ATTR(17, ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_POWER_LIMIT, ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE, cluster->overload_power_limit);
    SETUP_ATTR(18, ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_CURRENT_LIMIT, ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE, cluster->overload_current_limit);
    SETUP_ATTR(19, ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_TRIP_DELAY, ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE, cluster->overload_trip_delay);
    SETUP_ATTR(20, ZCL_ATTR_ELEC_MEAS_CUST_OVERVOLTAGE_WARN, ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE, cluster->overvoltage_warn);
    SETUP_ATTR(21, ZCL_ATTR_ELEC_MEAS_CUST_UNDERVOLTAGE_WARN, ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE, cluster->undervoltage_warn);
    SETUP_ATTR(22, ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_RECONNECT_DELAY, ZCL_DATA_TYPE_UINT16,
               ATTR_WRITABLE, cluster->overload_reconnect_delay);
    SETUP_ATTR(23, ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_ALARM, ZCL_DATA_TYPE_ENUM8,
               ATTR_READONLY, cluster->overload_alarm);
    SETUP_ATTR(24, ZCL_ATTR_ELEC_MEAS_APPARENT_POWER, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, cluster->apparent_power);
    SETUP_ATTR(25, ZCL_ATTR_ELEC_MEAS_REACTIVE_POWER, ZCL_DATA_TYPE_INT16,
               ATTR_READONLY, cluster->reactive_power);
    SETUP_ATTR(26, ZCL_ATTR_ELEC_MEAS_POWER_FACTOR, ZCL_DATA_TYPE_INT8,
               ATTR_READONLY, cluster->power_factor);

    endpoint->clusters[endpoint->cluster_count].cluster_id =
        ZCL_CLUSTER_ELECTRICAL_MEASUREMENT;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 27;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->cluster_count++;
    cluster->endpoint = endpoint->endpoint;
    g_elec_cluster    = cluster;
    electrical_measurement_cluster_load_calibration(cluster);
    // Seed the readable multiplier string (defaults, config_str markers and
    // NVM-restored calibration are all applied by now).
    elec_meas_refresh_calibration_values(cluster);
    // Restore persisted overload config (keeps defaults on first boot).
    elec_meas_overload_load(cluster);
    elec_meas_overload_mirror_to_attrs(cluster);
}

void electrical_measurement_cluster_load_calibration(
    electrical_measurement_cluster_t *cluster) {
    if (!cluster || !cluster->meter)
        return;

    elec_meas_cal_nv_t nv;
    if (hal_nvm_read(NV_ITEM_ENERGY_CALIBRATION, sizeof(nv), (uint8_t *)&nv) !=
        HAL_NVM_SUCCESS)
        return;

    if (nv.magic != ELEC_MEAS_CAL_NV_MAGIC)
        return;

    // Overrides the compile-time / config_str defaults with the field-tuned
    // values (0 = keep, so a partially-calibrated device is fine).
    energy_meter_set_calibration(cluster->meter, nv.voltage_multiplier,
                                 nv.current_multiplier, nv.power_multiplier);
    printf("ElecMeas: restored calibration V=%u A=%u W=%u\r\n",
           nv.voltage_multiplier, nv.current_multiplier, nv.power_multiplier);
}

void electrical_measurement_cluster_callback_attr_write_trampoline(
    uint8_t endpoint, uint16_t attribute_id) {
    electrical_measurement_cluster_t *cluster = g_elec_cluster;

    if (!cluster || cluster->endpoint != endpoint || !cluster->meter)
        return;

    // Calibrate the addressed channel, then clear the input field so it resets
    // and a repeat calibration re-triggers. (The power reference is uint32, so
    // the fields are cleared per-case rather than through a shared pointer.)
    int calibrated = -1;
    switch (attribute_id) {
    case ZCL_ATTR_ELEC_MEAS_CUST_CALIBRATE_VOLTAGE:
        calibrated = energy_meter_calibrate(cluster->meter,
                                            ENERGY_METER_CHANNEL_VOLTAGE,
                                            cluster->calibrate_voltage);
        cluster->calibrate_voltage = 0;
        break;
    case ZCL_ATTR_ELEC_MEAS_CUST_CALIBRATE_CURRENT:
        calibrated = energy_meter_calibrate(cluster->meter,
                                            ENERGY_METER_CHANNEL_CURRENT,
                                            cluster->calibrate_current);
        cluster->calibrate_current = 0;
        break;
    case ZCL_ATTR_ELEC_MEAS_CUST_CALIBRATE_POWER:
        calibrated = energy_meter_calibrate(cluster->meter,
                                            ENERGY_METER_CHANNEL_POWER,
                                            cluster->calibrate_power);
        cluster->calibrate_power = 0;
        break;
    case ZCL_ATTR_ELEC_MEAS_CUST_CALIBRATION_VALUES:
        // Direct multiplier import (e.g. copied from a calibrated reference
        // device). Applies, persists, and rewrites the canonical string.
        elec_meas_apply_calibration_values(cluster);
        elec_meas_refresh_calibration_values(cluster);
        return;

    case ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_POWER_LIMIT:
    case ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_CURRENT_LIMIT:
    case ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_TRIP_DELAY:
    case ZCL_ATTR_ELEC_MEAS_CUST_OVERVOLTAGE_WARN:
    case ZCL_ATTR_ELEC_MEAS_CUST_UNDERVOLTAGE_WARN:
    case ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_RECONNECT_DELAY:
        // Any overload setting changed: adopt, clamp, persist.
        elec_meas_overload_apply_attrs(cluster);
        return;

    default:
        return;
    }

    if (calibrated == 0) {
        elec_meas_save_calibration(cluster);
        elec_meas_refresh_calibration_values(cluster);
    }
}

void electrical_measurement_cluster_update(electrical_measurement_cluster_t *cluster) {
    if (!cluster || !cluster->meter)
        return;

    energy_meter_data_t data;
    energy_meter_get_data(cluster->meter, &data);
    if (data.valid) {
        cluster->rms_voltage  = data.voltage;
        cluster->rms_current  = data.current;
        cluster->active_power = data.power;
        cluster->freq_cf      = data.freq_cf;
        cluster->freq_cf1     = data.freq_cf1;
        cluster->sel_state    = data.sel_state;
        elec_meas_derive_power(data.voltage, data.current, data.power,
                               &cluster->apparent_power,
                               &cluster->reactive_power, &cluster->power_factor);
    }

    elec_meas_run_overload_protection(cluster, &data);
}

// Feed the latest measurement to the overload state machine and apply its
// verdict to the guarded relay. Uses the fastest available power estimate so a
// rising load trips quickly (see energy_meter_get_instant_power).
static void elec_meas_run_overload_protection(
    electrical_measurement_cluster_t *cluster, const energy_meter_data_t *data) {
    zigbee_relay_cluster *relay = (zigbee_relay_cluster *)cluster->protected_relay;

    if (!relay || !data->valid)
        return;

    int32_t           power  = energy_meter_get_instant_power(cluster->meter);
    overload_action_t action = overload_protection_check(
        &cluster->overload, hal_millis(), data->voltage, data->current, power,
        relay->relay->on, relay->startup_mode);

    if (action == OVERLOAD_ACTION_TURN_OFF && relay->relay->on) {
        relay_cluster_off(relay);
    } else if (action == OVERLOAD_ACTION_TURN_ON && !relay->relay->on) {
        relay_cluster_on(relay);
    }

    if (cluster->overload_alarm != (uint8_t)cluster->overload.alarm) {
        cluster->overload_alarm = (uint8_t)cluster->overload.alarm;
        hal_zigbee_notify_attribute_changed(
            cluster->endpoint, ZCL_CLUSTER_ELECTRICAL_MEASUREMENT,
            ZCL_ATTR_ELEC_MEAS_CUST_OVERLOAD_ALARM);
    }
}

void electrical_measurement_cluster_report(electrical_measurement_cluster_t *cluster) {
    // No autonomous reporting: attribute values are kept current by
    // electrical_measurement_cluster_update(), and the SDK's report_handler()
    // sends reports according to the coordinator's configureReporting settings
    // (min/max interval and reportable change), so reporting is fully
    // controllable from Z2M.
    (void)cluster;
}
