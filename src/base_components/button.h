#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "types.h"

typedef void (*ev_button_callback_t)(void *);
typedef void (*ev_button_multi_press_callback_t)(void *, u8);

typedef struct
{
  u32                              pin;
  u8                               gpio_last_state;
  u8                               pressed;
  u8                               long_pressed;
  u32                              pressed_at_ms;
  u32                              released_at_ms;
  u32                              long_press_duration_ms;
  u32                              multi_press_duration_ms;
  u8                               multi_press_cnt;
  u8                               debounce_last_state;
  u32                              debounce_last_change;
  ev_button_callback_t             on_press;
  ev_button_callback_t             on_long_press;
  ev_button_callback_t             on_release;
  ev_button_multi_press_callback_t on_multi_press;
  void *                           callback_param;
}button_t;


void btn_update(button_t *button);

#endif
