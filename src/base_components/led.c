#include "led.h"

#include "hal/gpio.h"
#include "hal/tasks.h"
#include "hal/timer.h"

#include <stdio.h>

void led_init(led_t *led) { led_off(led); }

void led_on(led_t *led) {
  hal_gpio_write(led->pin, led->on_high);
  led->on = 1;
  led->blink_times_left = 0;
}

void led_off(led_t *led) {
  hal_gpio_write(led->pin, !led->on_high);
  led->on = 0;
  led->blink_times_left = 0;
}

static void led_blink_handler(void *arg) {
  led_t *led = (led_t *)arg;
  if (led->blink_times_left == 0)
    return;
  if (led->on) {
    led->on = 0;
    hal_gpio_write(led->pin, !led->on_high);
    if (led->blink_times_left != LED_BLINK_FOREVER) {
      led->blink_times_left--;
    }
    hal_tasks_schedule(&led->blink_task, led->blink_time_off);
  } else {
    led->on = 1;
    hal_gpio_write(led->pin, led->on_high);
    hal_tasks_schedule(&led->blink_task, led->blink_time_on);
  }
}

void led_blink(led_t *led, uint16_t on_time_ms, uint16_t off_time_ms,
               uint16_t times) {
  if (led->blink_times_left != 0) {
    led->blink_times_left = times;
    return;
  }

  hal_gpio_write(led->pin, led->on_high);
  led->on = 1;
  led->blink_time_on = on_time_ms;
  led->blink_time_off = off_time_ms;
  led->blink_times_left = times;
  led->blink_task.handler = led_blink_handler;
  led->blink_task.arg = led;
  hal_tasks_init(&led->blink_task);
  hal_tasks_schedule(&led->blink_task, on_time_ms);
}
