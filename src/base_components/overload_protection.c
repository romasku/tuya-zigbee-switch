#include "overload_protection.h"

// StartUpOnOff value that means "power on to OFF" (ZCL 0x00). Any other value
// (ON=0x01, TOGGLE=0x02, PREVIOUS=0xFF) is treated as "may auto-reconnect".
#define STARTUP_MODE_OFF    0x00

void overload_protection_init(overload_protection_t *op) {
    if (!op)
        return;

    op->cfg.power_limit_w     = 2500;                     // ~10 A continuous rating
    op->cfg.current_limit_ma  = 10000;                    // 10 A continuous rating
    op->cfg.trip_delay_s      = 30;
    op->cfg.overvoltage_cv    = 26000;                    // 260 V
    op->cfg.undervoltage_cv   = 21000;                    // 210 V
    op->cfg.reconnect_delay_s = 60;
    op->cfg.hard_power_w      = OVERLOAD_HARD_POWER_W;    // 3680 W peak
    op->cfg.hard_current_ma   = OVERLOAD_HARD_CURRENT_MA; // 16 A peak

    op->alarm           = OVERLOAD_ALARM_NONE;
    op->tripped         = 0;
    op->locked_out      = 0;
    op->retry_count     = 0;
    op->over_since_ms   = 0;
    op->reconnect_at_ms = 0;
}

void overload_protection_set_current_limits(overload_protection_t *op,
                                            uint16_t soft_current_ma,
                                            uint16_t hard_current_ma) {
    if (!op)
        return;

    // P = I * V; current in mA * V / 1000 = W (result fits uint16 for mains
    // loads: 20 A * 230 V = 4600 W).
    if (soft_current_ma) {
        op->cfg.current_limit_ma = soft_current_ma;
        op->cfg.power_limit_w    = (uint16_t)((uint32_t)soft_current_ma *
                                              OVERLOAD_NOMINAL_VOLTAGE_V / 1000u);
    }
    if (hard_current_ma) {
        op->cfg.hard_current_ma = hard_current_ma;
        op->cfg.hard_power_w    = (uint16_t)((uint32_t)hard_current_ma *
                                             OVERLOAD_NOMINAL_VOLTAGE_V / 1000u);
    }
}

static uint32_t stamp(uint32_t now) {
    // 0 is used as the "unset" sentinel, so never store a literal 0.
    return now == 0 ? 1 : now;
}

static uint8_t reached(uint32_t now, uint32_t target) {
    return (int32_t)(now - target) >= 0;
}

// Turn the relay off for the given reason, arm auto-reconnect (or latch out
// after too many retries), and report TURN_OFF.
static overload_action_t trip(overload_protection_t *op, uint32_t now,
                              overload_alarm_t reason, uint8_t startup_mode) {
    op->tripped       = 1;
    op->over_since_ms = 0;

    if (op->retry_count >= OVERLOAD_MAX_RETRIES) {
        op->locked_out      = 1;
        op->alarm           = OVERLOAD_ALARM_LOCKED_OUT;
        op->reconnect_at_ms = 0;
    } else if (startup_mode != STARTUP_MODE_OFF) {
        op->alarm           = reason;
        op->reconnect_at_ms = stamp(now + op->cfg.reconnect_delay_s * 1000u);
    } else {
        op->alarm           = reason;
        op->reconnect_at_ms = 0; // power-on-behaviour OFF: stay off till manual
    }
    return OVERLOAD_ACTION_TURN_OFF;
}

overload_action_t overload_protection_check(overload_protection_t *op,
                                            uint32_t now_ms, uint16_t voltage_cv,
                                            uint16_t current_ma, int32_t power_w,
                                            uint8_t relay_is_on,
                                            uint8_t startup_mode) {
    if (!op)
        return OVERLOAD_ACTION_NONE;

    // Manual re-enable: it was tripped but is on again without us reconnecting
    // (auto-reconnect clears `tripped` when it issues TURN_ON). Re-arm.
    if (op->tripped && relay_is_on) {
        op->tripped         = 0;
        op->locked_out      = 0;
        op->retry_count     = 0;
        op->over_since_ms   = 0;
        op->reconnect_at_ms = 0;
        op->alarm           = OVERLOAD_ALARM_NONE;
    }

    if (op->locked_out) {
        op->alarm = OVERLOAD_ALARM_LOCKED_OUT;
        return relay_is_on ? OVERLOAD_ACTION_TURN_OFF : OVERLOAD_ACTION_NONE;
    }

    // Tripped and still off: wait for (and honour) the auto-reconnect policy.
    if (op->tripped && !relay_is_on) {
        if (startup_mode != STARTUP_MODE_OFF && op->reconnect_at_ms != 0 &&
            reached(now_ms, op->reconnect_at_ms)) {
            op->retry_count++;
            op->tripped         = 0;
            op->over_since_ms   = 0;
            op->reconnect_at_ms = 0;
            return OVERLOAD_ACTION_TURN_ON;
        }
        return OVERLOAD_ACTION_NONE;
    }

    // Relay is off and not tripped: user turned it off, nothing to protect.
    if (!relay_is_on) {
        op->over_since_ms = 0;
        op->alarm         = OVERLOAD_ALARM_NONE;
        return OVERLOAD_ACTION_NONE;
    }

    // Relay is on: evaluate the load against the device's peak caps (0 = off).
    uint8_t peak = (op->cfg.hard_power_w != 0 &&
                    power_w >= (int32_t)op->cfg.hard_power_w) ||
                   (op->cfg.hard_current_ma != 0 &&
                    current_ma >= op->cfg.hard_current_ma);
    if (peak) {
        return trip(op, now_ms, OVERLOAD_ALARM_PEAK, startup_mode);
    }

    uint8_t soft_power =
        (op->cfg.power_limit_w != 0) && (power_w >= (int32_t)op->cfg.power_limit_w);
    uint8_t soft_current =
        (op->cfg.current_limit_ma != 0) && (current_ma >= op->cfg.current_limit_ma);

    if (soft_power || soft_current) {
        if (op->over_since_ms == 0) {
            op->over_since_ms = stamp(now_ms);
        }
        op->alarm = soft_power ? OVERLOAD_ALARM_POWER : OVERLOAD_ALARM_CURRENT;
        if ((uint32_t)(now_ms - op->over_since_ms) >=
            op->cfg.trip_delay_s * 1000u) {
            return trip(op, now_ms, op->alarm, startup_mode);
        }
        return OVERLOAD_ACTION_NONE; // warning; grace timer still running
    }

    // Within limits: clear the grace timer and surface voltage warnings only.
    op->over_since_ms = 0;
    if (op->cfg.overvoltage_cv != 0 && voltage_cv > op->cfg.overvoltage_cv) {
        op->alarm = OVERLOAD_ALARM_VOLTAGE_HIGH;
    } else if (op->cfg.undervoltage_cv != 0 &&
               voltage_cv > OVERLOAD_VOLTAGE_FLOOR_CV &&
               voltage_cv < op->cfg.undervoltage_cv) {
        op->alarm = OVERLOAD_ALARM_VOLTAGE_LOW;
    } else {
        op->alarm = OVERLOAD_ALARM_NONE;
    }
    return OVERLOAD_ACTION_NONE;
}
