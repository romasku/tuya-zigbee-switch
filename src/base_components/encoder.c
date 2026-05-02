#include "encoder.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include <stdbool.h>
#include <stddef.h>

static void sw_callback(hal_gpio_pin_t pin, encoder_t *encoder);
static void rotate_callback(hal_gpio_pin_t pin, encoder_t *encoder);

void encoder_init(encoder_t *encoder) {
    printf("Encoder Init, with PinA: %d, PinB: %d, PinSW: %d\r\n", encoder->pin_a, encoder->pin_b,
           encoder->pin_sw);

    encoder->pin_sw_state       = hal_gpio_read(encoder->pin_sw);
    encoder->pin_sw_last_change = hal_millis();

    encoder->old_AB = 0b00000011; // TODO, we are assuming pin a & b start high. We should read their actual values.
    encoder->encval = 0;

    hal_gpio_callback(encoder->pin_a, (gpio_callback_t)rotate_callback, encoder);
    hal_gpio_callback(encoder->pin_b, (gpio_callback_t)rotate_callback, encoder);
    hal_gpio_callback(encoder->pin_sw, (gpio_callback_t)sw_callback, encoder);

    encoder->rotate_since_pressed = false;
}

// Based on https://youtube.com/watch?v=fgOfSHTYeio
// NOTE: Using a half step encoder (detent on high and low), so step size (encval) is reduced from 4 to 2 per step
static void rotate_callback(hal_gpio_pin_t pin, encoder_t *encoder) {
    static const int8_t enc_states[] = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

    encoder->old_AB <<= 2;                                             // Push old pin values into into high positions IE 0b00000011 -> 0b00001100

    if (hal_gpio_read(encoder->pin_a)) encoder->old_AB |= 0b00000010;  // Read pin a state into position 2
    if (hal_gpio_read(encoder->pin_b)) encoder->old_AB |= 0b00000001;  // Read pin b state into position 1

    encoder->encval += enc_states[(encoder->old_AB & 0b00001111)];     // Use last four bits to lookup change in enc_state and apply to encval

    if (encoder->encval > 1) {
        encoder->encval = 0;
        encoder->rotate_since_pressed = true;

        if (encoder->pin_sw_state == 0) {
            printf("Encoder Rotating CW while Pressed\r\n");
            if (encoder->on_rotate_cw_while_pressed != NULL) {
                encoder->on_rotate_cw_while_pressed(encoder->callback_param);
            }
        }else{
            printf("Encoder Rotating CW\r\n");

            if (encoder->on_rotate_cw != NULL) {
                encoder->on_rotate_cw(encoder->callback_param);
            }
        }
    }else if (encoder->encval < -1) {
        encoder->encval = 0;
        encoder->rotate_since_pressed = true;

        if (encoder->pin_sw_state == 0) {
            printf("Encoder Rotating CCW while Pressed\r\n");
            if (encoder->on_rotate_ccw_while_pressed != NULL) {
                encoder->on_rotate_ccw_while_pressed(encoder->callback_param);
            }
        }else{
            printf("Encoder Rotating CCW\r\n");
            if (encoder->on_rotate_ccw != NULL) {
                encoder->on_rotate_ccw(encoder->callback_param);
            }
        }
    }
}

static void sw_callback(hal_gpio_pin_t pin, encoder_t *encoder) {
    uint8_t  new_state = hal_gpio_read(encoder->pin_sw);
    uint32_t now       = hal_millis();

    if (new_state != encoder->pin_sw_state &&
        (now - encoder->pin_sw_last_change) > 10) {
        encoder->pin_sw_state       = new_state;
        encoder->pin_sw_last_change = now;

        if (new_state == 0) {
            printf("Encoder Pressed\r\n");

            encoder->rotate_since_pressed = false;
        } else {
            printf("Encoder Released\r\n");

            if (!encoder->rotate_since_pressed && encoder->on_press != NULL)
                encoder->on_press(encoder->callback_param);
        }
    }
}
