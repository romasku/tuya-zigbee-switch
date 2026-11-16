#ifndef _HAL_TICK64_H_
#define _HAL_TICK64_H_

#include <stdint.h>

/**
 * Wrap-safe millisecond counter for a 16 MHz free-running tick.
 *
 * clock_time() is a 32-bit counter clocked at 16 MHz, so it wraps every
 * 2^32 / 16e6 = 268.435456 s.
 *
 * The old hal_millis() divided the raw tick by 16000 and tracked the wrap
 * of that derived value. The derived value only spans 0..268435 ms, so the
 * wrap counter was shifted left by 32 bits: every hardware wrap added
 * 2^32 ms (~49.7 days) instead of the ~268 s that really elapsed. The
 * result was not a millisecond timeline, and every deadline longer than
 * one wrap was reached at the wrap boundary instead of at its deadline.
 *
 * The counter below advances by the elapsed tick delta instead. The delta
 * is a plain uint32 subtraction, so it stays correct across the wrap.
 * The sub-millisecond remainder is carried over, because a tick period is
 * 268435.456 ms rather than a whole number of milliseconds.
 *
 * Only 32-bit division is used: the tc32 toolchain has no __udivdi3.
 *
 * Deadlines of any length compare correctly, up to the ZCL On With Timed
 * Off maximum of 6553 s.
 *
 * The header is dependency-free so the firmware and the regression test
 * compile the same code.
 */

/** Ticks per millisecond for the 16 MHz system timer. */
#define HAL_TICK64_TICKS_PER_MS    16000

/** State of the counter. */
typedef struct {
    uint32_t last;    /**< raw tick seen on the previous read */
    uint32_t frac;    /**< ticks carried over, always < HAL_TICK64_TICKS_PER_MS */
    uint64_t ms;      /**< whole milliseconds since boot */
    uint8_t  started; /**< set after the first read */
} hal_tick64_state;

/** Reset the state. Call once before the first use. */
static inline void hal_tick64_init(hal_tick64_state *state) {
    state->last    = 0;
    state->frac    = 0;
    state->ms      = 0;
    state->started = 0;
}

/**
 * Advance the counter to the given raw tick and return the elapsed
 * milliseconds since boot.
 *
 * The first read establishes the origin from the tick itself, so the
 * result is boot-relative from the start. Every later read adds the
 * elapsed interval.
 */
static inline uint64_t hal_tick64_millis(hal_tick64_state *state, uint32_t now) {
    if (!state->started) {
        // First read: the raw tick already carries the time since boot.
        state->started = 1;
        state->last    = now;
        state->frac    = now % HAL_TICK64_TICKS_PER_MS;
        state->ms      = now / HAL_TICK64_TICKS_PER_MS;
        return state->ms;
    }

    // Ticks since the previous read. Modulo 2^32 this is the true elapsed
    // tick count even across a hardware wrap, as long as less than one tick
    // period passes between reads.
    uint32_t delta = now - state->last;
    state->last = now;

    // Whole milliseconds in the delta, plus the carried remainder.
    uint32_t whole = delta / HAL_TICK64_TICKS_PER_MS;
    uint32_t rem   = state->frac + (delta % HAL_TICK64_TICKS_PER_MS);
    uint32_t carry = rem / HAL_TICK64_TICKS_PER_MS;
    state->frac = rem % HAL_TICK64_TICKS_PER_MS;

    state->ms += (uint64_t)whole + (uint64_t)carry;

    return state->ms;
}

#endif
