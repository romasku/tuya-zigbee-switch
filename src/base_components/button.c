#include "button.h"
#include "tl_common.h"
#include "millis.h"


bool btn_debounce(button_t *button, u8 is_pressed);
void btn_update_debounced(button_t *button, u8 is_pressed);


void btn_init(button_t *button)
{
  // During device startup, button may be already pressed, but this should not be detected
  // as user press. So, to avoid such situation, special init is required.
  u8 state = drv_gpio_read(button->pin);
  if (!state) {
     button->pressed = true;
     button->long_pressed = true;
  }
}

void btn_update(button_t *button)
{
  u8 state = drv_gpio_read(button->pin);

  if (btn_debounce(button, state))
  {
    btn_update_debounced(button, !state);
  }
}

bool btn_debounce(button_t *button, u8 is_pressed) {
  u32 now = millis();

  if (is_pressed != button->debounce_last_state)
  {
    button->debounce_last_state = is_pressed;
    button->debounce_last_change = now;
  }

  return (now - button->debounce_last_change) > DEBOUNCE_DELAY_MS;
}

void btn_update_debounced(button_t *button, u8 is_pressed)
{
  u32 now = millis();

  if (!button->pressed && is_pressed)
  {
    printf("Press detected\r\n");
    button->pressed_at_ms = now;
    if (button->on_press != NULL)
    {
      button->on_press(button->callback_param);
    }
    if (now - button->released_at_ms < button->multi_press_duration_ms)
    {
      button->multi_press_cnt += 1;
      printf("Multi press detected: %d\r\n", button->multi_press_cnt);
      if (button->on_multi_press != NULL)
      {
        button->on_multi_press(button->callback_param, button->multi_press_cnt);
      }
    }
    else
    {
      button->multi_press_cnt = 1;
    }
  }
  else if (button->pressed && !is_pressed)
  {
    printf("Release detected\r\n");
    button->released_at_ms = now;
    button->long_pressed   = false;
    if (button->on_release != NULL)
    {
      button->on_release(button->callback_param);
    }
  }
  button->pressed = is_pressed;
  if (is_pressed && !button->long_pressed && (button->long_press_duration_ms > 0) && (button->long_press_duration_ms < (now - button->pressed_at_ms)))
  {
    button->long_pressed = true;
    printf("Long press detected\r\n");
    if (button->on_long_press != NULL)
    {
      button->on_long_press(button->callback_param);
    }
  }
  ;
}
