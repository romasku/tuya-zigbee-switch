#include "relay.h"
#include "hal/gpio.h"
#include "hal/printf_selector.h"
#include "hal/tasks.h"
#include <stddef.h>

#ifndef RELAY_PULSE_MS
#define RELAY_PULSE_MS 125
#endif

#ifndef BISTABLE_WAIT_END_MS
#define BISTABLE_WAIT_END_MS 50
#endif

static relay_t *pulse_relay = NULL;

static void relay_start_bistable_pulse(relay_t *relay);
static void relay_end_bistable_pulse(relay_t *relay);

static void relay_end_bistable_pulse_handler(void *arg);
static void relay_start_bistable_pulse_handler(void *arg);

static void relay_end_bistable_pulse_handler(void *arg) {
  printf("relay_clear_handler %d\r\n", arg);
  relay_end_bistable_pulse((relay_t *)arg);
}

static void relay_start_bistable_pulse_handler(void *arg) {
  printf("relay_start_handler\r\n");
  relay_start_bistable_pulse((relay_t *)arg);
}

static void relay_end_bistable_pulse(relay_t *relay) {
  hal_gpio_write(relay->pin, !relay->on_high);
  hal_gpio_write(relay->off_pin, !relay->on_high);
  if (pulse_relay == relay) {
    // If this relay had a pending pulse, mark it as cleared
    pulse_relay = NULL;
  }
}

static void relay_start_bistable_pulse(relay_t *relay) {
  hal_gpio_pin_t pin = relay->on ? relay->pin : relay->off_pin;

  if (pulse_relay == NULL) {
    // Start new pulse
    hal_gpio_write(pin, relay->on_high);
    pulse_relay = relay;
    relay->bistable_task.handler = relay_end_bistable_pulse_handler;
    hal_tasks_schedule(&relay->bistable_task, RELAY_PULSE_MS);
  } else {
    printf("relay_start_bistable_pulse: another pulse is active\r\n");
    relay->bistable_task.handler = relay_start_bistable_pulse_handler;
    hal_tasks_schedule(&relay->bistable_task, BISTABLE_WAIT_END_MS);
  }
}

void relay_init(relay_t *relay) {
  relay->bistable_task.arg = relay;
  hal_tasks_init(&relay->bistable_task);

  // switch off relay
  if (!relay->off_pin) {
    // Normal relay
    hal_gpio_write(relay->pin, !relay->on_high);
  } else {
    // Bi-stable relay
    hal_gpio_write(relay->pin, !relay->on_high);
    hal_gpio_write(relay->off_pin, !relay->on_high);
  }
}

void relay_on(relay_t *relay) {
  if (relay == NULL) {
    return;
  }
  printf("relay_on\r\n");

  relay->on = 1;
  if (!relay->off_pin) {
    // Normal relay: drive continuously
    hal_gpio_write(relay->pin, relay->on_high);
  } else {
    // Bi-stable relay
    relay_end_bistable_pulse(relay);
    hal_tasks_unschedule(&relay->bistable_task);
    relay_start_bistable_pulse(relay);
  }

  if (relay->on_change != NULL) {
    relay->on_change(relay->callback_param, 1);
  }
}

void relay_off(relay_t *relay) {
  if (relay == NULL) {
    return;
  }
  printf("relay_off\r\n");

  // Clear both pins
  hal_gpio_write(relay->pin, !relay->on_high);

  relay->on = 0;
  if (!relay->off_pin) {
    // Normal relay
    hal_gpio_write(relay->pin, !relay->on_high);
  } else {
    // Bi-stable relay
    relay_end_bistable_pulse(relay);
    hal_tasks_unschedule(&relay->bistable_task);
    relay_start_bistable_pulse(relay);
  }

  if (relay->on_change != NULL) {
    relay->on_change(relay->callback_param, 0);
  }
}

void relay_toggle(relay_t *relay) {
  if (relay == NULL) {
    return;
  }
  printf("relay_toggle\r\n");

  if (relay->on) {
    relay_off(relay);
  } else {
    relay_on(relay);
  }
}
