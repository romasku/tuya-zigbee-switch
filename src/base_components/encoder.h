#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "hal/gpio.h"
#include "hal/tasks.h"
#include <stdint.h>
#include <stdbool.h>

typedef void (*ev_encoder_callback_t)(void *);
typedef void (*ev_encoder_multi_press_callback_t)(void *, uint8_t);

typedef struct {
    hal_gpio_pin_t        pin_a; // Also known as CLK
    hal_gpio_pin_t        pin_b; // Also known as DT
    uint8_t               old_AB;
    int8_t                encval;

    hal_gpio_pin_t        pin_sw;
    uint8_t               pin_sw_state;
    uint32_t              pin_sw_last_change;

    bool                  rotate_since_pressed;

    ev_encoder_callback_t on_press;
    ev_encoder_callback_t on_rotate_ccw;
    ev_encoder_callback_t on_rotate_cw;
    ev_encoder_callback_t on_rotate_ccw_while_pressed;
    ev_encoder_callback_t on_rotate_cw_while_pressed;
    void *                callback_param;

    // Needed to detect Multi presses for reset logic
    uint32_t              released_at_ms;
    uint32_t              multi_press_duration_ms;
    uint8_t               multi_press_cnt;
    ev_encoder_multi_press_callback_t on_multi_press;
} encoder_t;

void encoder_init(encoder_t *encoder);

#endif
