#ifndef _ENERGY_METER_H_
#define _ENERGY_METER_H_

#include <stdint.h>

typedef struct energy_meter energy_meter_t;

typedef enum {
    ENERGY_METER_NONE    = 0,
    ENERGY_METER_HLW8012 = 1,
    ENERGY_METER_BL0937  = 2,
    ENERGY_METER_BL0942  = 3,
} energy_meter_type_t;

typedef struct {
    uint16_t voltage; // cV
    uint16_t current; // mA
    int16_t  power;   // W
    uint32_t energy;  // Wh
    uint8_t  valid;
    uint32_t freq_cf;
    uint32_t freq_cf1;
    uint8_t  sel_state;
} energy_meter_data_t;

// Calibration channels used by energy_meter_calibrate().
typedef enum {
    ENERGY_METER_CHANNEL_VOLTAGE = 0,
    ENERGY_METER_CHANNEL_CURRENT = 1,
    ENERGY_METER_CHANNEL_POWER   = 2,
} energy_meter_channel_t;

typedef struct {
    uint32_t voltage_multiplier;
    uint32_t current_multiplier;
    uint32_t power_multiplier;
} energy_meter_calibration_t;

typedef struct {
    void (*get_data)(void *ctx, energy_meter_data_t *data);
    void (*reset_energy)(void *ctx);
    void (*tick)(void *ctx);
    // Calibrate a channel so it currently reads `reference` (in the meter's
    // native unit: voltage cV, current mA, power W). Returns 0 on success.
    int (*calibrate)(void *ctx, energy_meter_channel_t channel,
                     uint32_t reference);
    // Read back the current calibration multipliers (for persistence).
    void (*get_calibration)(void *ctx, energy_meter_calibration_t *cal);
    // Force-set calibration multipliers (0 = keep current). Used to restore
    // persisted calibration on boot.
    void (*set_calibration)(void *ctx, uint32_t voltage_mult,
                            uint32_t current_mult, uint32_t power_mult);
    // Optional: fastest available power estimate (W), for overload protection.
    // NULL means "no faster value than get_data()'s power".
    int32_t (*get_instant_power)(void *ctx);
} energy_meter_ops_t;

struct energy_meter {
    const energy_meter_ops_t *ops;
    void *                    ctx;
    energy_meter_type_t       type;
};

static inline void energy_meter_init(energy_meter_t *meter,
                                     const energy_meter_ops_t *ops, void *ctx,
                                     energy_meter_type_t type) {
    if (!meter)
        return;

    meter->ops  = ops;
    meter->ctx  = ctx;
    meter->type = type;
}

static inline void energy_meter_get_data(energy_meter_t *meter,
                                         energy_meter_data_t *data) {
    if (meter && meter->ops && meter->ops->get_data && data)
        meter->ops->get_data(meter->ctx, data);
}

static inline void energy_meter_reset_energy(energy_meter_t *meter) {
    if (meter && meter->ops && meter->ops->reset_energy)
        meter->ops->reset_energy(meter->ctx);
}

static inline void energy_meter_tick(energy_meter_t *meter) {
    if (meter && meter->ops && meter->ops->tick)
        meter->ops->tick(meter->ctx);
}

static inline int energy_meter_is_valid(energy_meter_t *meter) {
    return meter && meter->ops && meter->ctx;
}

static inline int energy_meter_calibrate(energy_meter_t *meter,
                                         energy_meter_channel_t channel,
                                         uint32_t reference) {
    if (meter && meter->ops && meter->ops->calibrate)
        return meter->ops->calibrate(meter->ctx, channel, reference);

    return -1;
}

static inline void energy_meter_get_calibration(
    energy_meter_t *meter, energy_meter_calibration_t *cal) {
    if (meter && meter->ops && meter->ops->get_calibration && cal)
        meter->ops->get_calibration(meter->ctx, cal);
}

// Fastest available power reading (W) for protection; falls back to the last
// get_data() power when the driver has no faster estimate.
static inline int32_t energy_meter_get_instant_power(energy_meter_t *meter) {
    if (meter && meter->ops && meter->ops->get_instant_power)
        return meter->ops->get_instant_power(meter->ctx);

    if (meter && meter->ops && meter->ops->get_data) {
        energy_meter_data_t data;
        meter->ops->get_data(meter->ctx, &data);
        return data.valid ? data.power : 0;
    }
    return 0;
}

static inline void energy_meter_set_calibration(energy_meter_t *meter,
                                                uint32_t voltage_mult,
                                                uint32_t current_mult,
                                                uint32_t power_mult) {
    if (meter && meter->ops && meter->ops->set_calibration)
        meter->ops->set_calibration(meter->ctx, voltage_mult, current_mult,
                                    power_mult);
}

#endif /* _ENERGY_METER_H_ */
