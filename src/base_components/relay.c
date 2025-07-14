#include "relay.h"
#include "tl_common.h"
#include "millis.h"

#define MAX_RELAY_PULSES 2  // Number of pulses allowed at the same time

typedef struct {
  u32 pin; 
  u8 turn_off;
} relay_pulse_t;

static relay_pulse_t pulse_pool[MAX_RELAY_PULSES];
static u8 pulse_pool_in_use[MAX_RELAY_PULSES];
static void deactivate_pin(u32 pin, u8 turn_off);
static void schedule_clear_pin(void *arg);

void relay_init(relay_t *relay)
{
  relay_off(relay);
}

void relay_on(relay_t *relay)
{
    printf("relay_on\r\n");

    if (relay->off_pin) {
        // Bi-stable relay: activate the ON pin
        drv_gpio_write(relay->pin, relay->on_high);
        // and schedule a pulse to clear the ON pin
        deactivate_pin(relay->pin, !relay->on_high);
    } else {
        // Normal relay: drive continuously
        drv_gpio_write(relay->pin, relay->on_high);
    }

    relay->on = 1;

    if (relay->on_change) {
        relay->on_change(relay->callback_param, 1);
    }
}


void relay_off(relay_t *relay)
{
    printf("relay_off\r\n");

    if (relay->off_pin) {
        // Bi-stable relay: activate the OFF pin
        drv_gpio_write(relay->off_pin, relay->on_high);
        // and schedule a pulse to clear the OFF pin
        deactivate_pin(relay->off_pin, !relay->on_high);
    } else {
        // Normal relay: turn OFF
        drv_gpio_write(relay->pin, !relay->on_high);
    }

    relay->on = 0;

    if (relay->on_change) {
        relay->on_change(relay->callback_param, 0);
    }
}


void relay_toggle(relay_t *relay)
{
  printf("relay_toggle\r\n");
  if (relay->on)
  {
    relay_off(relay);
  }
  else
  {
    relay_on(relay);
  }
}

static void deactivate_pin(u32 pin, u8 turn_off)
{
  printf("deactivate_pin\r\n");

  relay_pulse_t *pulse = pulse_alloc();
  if (!pulse) return;

  pulse->pin = pin;
  pulse->turn_off = turn_off;

  TL_ZB_TIMER_SCHEDULE(schedule_pin_clear, pulse, 125);
}


static void schedule_pin_clear(void *arg)
{
  printf("schedule_pin clear\r\n");
  relay_pulse_t *pulse = (relay_pulse_t *)arg;
  drv_gpio_write(pulse->pin, pulse->turn_off);
  pulse_free(pulse);
}

static relay_pulse_t* pulse_alloc(void) {
  for (int i = 0; i < MAX_RELAY_PULSES; ++i) {
    if (!pulse_pool_in_use[i]) {
      pulse_pool_in_use[i] = 1;
      return &pulse_pool[i];
    }
  }
  return NULL;
}

static void pulse_free(relay_pulse_t* p) {
  for (int i = 0; i < MAX_RELAY_PULSES; ++i) {
    if (&pulse_pool[i] == p) {
      pulse_pool_in_use[i] = 0;
      return;
    }
  }
}