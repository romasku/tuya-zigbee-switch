#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "hal/gpio.h"
#include "hal/tasks.h"
#include <stdint.h>

typedef void (*ev_button_callback_t)(void *);
typedef void (*ev_button_multi_press_callback_t)(void *, uint8_t);

#define DEBOUNCE_DELAY_MS    0 // 0 = no software debounce (hardware cap)

typedef struct {
    hal_gpio_pin_t                   pin;
    uint8_t                          pressed_when_high;
    uint8_t                          pressed;
    uint8_t                          long_pressed;
    uint32_t                         pressed_at_ms;
    uint32_t                         released_at_ms;
    uint16_t                         long_press_duration_ms;
    uint32_t                         multi_press_duration_ms;
    uint8_t                          multi_press_cnt;
    uint16_t                         debounce_delay_ms; // 0 = skip (hw cap)
    hal_task_t                       update_task;
    uint8_t                          debounce_last_state;
    uint32_t                         debounce_last_change;
    uint8_t                          retention_debounce;
    ev_button_callback_t             on_press;
    ev_button_callback_t             on_long_press;
    ev_button_callback_t             on_release;
    ev_button_multi_press_callback_t on_multi_press;
    void *                           callback_param;
} button_t;

void btn_init(button_t *button);

/**
 * Call after deep retention wake to sync button state with actual GPIO level.
 * Detects press/release that happened across the sleep boundary and fires
 * the appropriate callbacks (on_press, on_release, on_multi_press).
 */
void btn_retention_wake(button_t *button);

#endif
