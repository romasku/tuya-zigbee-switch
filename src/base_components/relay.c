#include "relay.h"
#include "hal/gpio.h"
#include "hal/tasks.h"
#include "hal/printf_selector.h"
#include <stddef.h>

#ifndef RELAY_PULSE_MS
#define RELAY_PULSE_MS 125
#endif

// Handler to clear both pins after pulse
static void relay_clear_handler(void *arg)
{
  printf("relay_clear_handler\r\n");

  relay_t *relay = (relay_t *)arg;

  // Check if task is still active (not superseded)
  if (!relay->clear_task_active)
  {
    return;
  }
    
  // Clear both pins to ensure mutual exclusion
  hal_gpio_write(relay->pin, !relay->on_high);
  if (relay->off_pin)
  {
    hal_gpio_write(relay->off_pin, !relay->on_high);
  }
  
  // Mark that no task is active
  relay->clear_task_active = 0;
}

void relay_init(relay_t *relay)
{
  if (relay == NULL)
  {
    return;
  }

  // Mark that no task is active
  relay->clear_task_active = 0;
  
  // Prepare the task structure *once*
  relay->clear_task.handler = relay_clear_handler;
  relay->clear_task.arg = relay;
  hal_tasks_init(&relay->clear_task);

  //switch off relay
  relay_off(relay);
}

void relay_on(relay_t *relay)
{
  if (relay == NULL)
  {
    return;
  }
  printf("relay_on\r\n");
  
  // Invalidate any pending task
  relay->clear_task_active = 0;

  if (!relay->off_pin)
  {
    // Normal relay: drive continuously
    hal_gpio_write(relay->pin, relay->on_high);
  } 
  else 
  {
    // Bi-stable relay
    hal_gpio_write(relay->pin, !relay->on_high);
    hal_gpio_write(relay->off_pin, !relay->on_high);

    hal_gpio_write(relay->pin, relay->on_high);

    // Schedule already-initialized task
    relay->clear_task_active = 1;
    hal_tasks_schedule(&relay->clear_task, RELAY_PULSE_MS);
  }
  
  relay->on = 1;
  
  if (relay->on_change != NULL)
  {
    relay->on_change(relay->callback_param, 1);
  }
}

void relay_off(relay_t *relay)
{
  if (relay == NULL)
  {
    return;
  }
  printf("relay_off\r\n");
  
  // Invalidate any pending task
  relay->clear_task_active = 0;
  
  // Clear both pins
  hal_gpio_write(relay->pin, !relay->on_high);

  if (!relay->off_pin)
  {
    // Normal relay
    hal_gpio_write(relay->pin, !relay->on_high);
  } 
  else 
  {
    // Bi-stable relay
    hal_gpio_write(relay->pin, !relay->on_high);
    hal_gpio_write(relay->off_pin, !relay->on_high);

    hal_gpio_write(relay->off_pin, relay->on_high);

    // Schedule already-initialized task
    relay->clear_task_active = 1;
    hal_tasks_schedule(&relay->clear_task, RELAY_PULSE_MS);
  }
  
  relay->on = 0;
  
  if (relay->on_change != NULL)
  {
    relay->on_change(relay->callback_param, 0);
  }
}

void relay_toggle(relay_t *relay)
{
  if (relay == NULL)
  {
    return;
  }
  printf("relay_toggle\r\n");

  if (relay->on)
  {
    relay_off(relay);
  } else {
    relay_on(relay);
  }
}
