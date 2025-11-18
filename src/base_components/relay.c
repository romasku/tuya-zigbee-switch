#include "relay.h"
#include "hal/gpio.h"
#include "hal/tasks.h"
#include "hal/printf_selector.h"
#include <stddef.h>

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
  // Mark that no task is active
  relay->clear_task_active = 0;
  //switch off relay
  relay_off(relay);
}

void relay_on(relay_t *relay)
{
  printf("relay_on\r\n");
  
  // Invalidate any pending task by clearing the flag
  relay->clear_task_active = 0;

  if (!relay->off_pin)
  {
    // Normal relay: drive continuously
    hal_gpio_write(relay->pin, relay->on_high);
  } else {
    //Bi-stable relay: 
    // clear both pins first
    hal_gpio_write(relay->pin, !relay->on_high);
    hal_gpio_write(relay->off_pin, !relay->on_high);
    // then pulse ON pin
    hal_gpio_write(relay->pin, relay->on_high);
    
    // Schedule task to clear both pins after 125ms
    relay->clear_task.handler = relay_clear_handler;
    relay->clear_task.arg = relay;
    hal_tasks_init(&relay->clear_task);
    relay->clear_task_active = 1;
    hal_tasks_schedule(&relay->clear_task, 125);
  }
  
  relay->on = 1;
  
  if (relay->on_change != NULL)
  {
    relay->on_change(relay->callback_param, 1);
  }
}

void relay_off(relay_t *relay)
{
  printf("relay_off\r\n");
  
  // Invalidate any pending task by clearing the flag
  relay->clear_task_active = 0;
  
  // Clear both pins first to ensure mutual exclusion
  hal_gpio_write(relay->pin, !relay->on_high);

  if (!relay->off_pin)
  {
    // Normal relay: turn OFF
    hal_gpio_write(relay->pin, !relay->on_high);
  } else {
    //Bi-stable relay: 
    // clear both pins first
    hal_gpio_write(relay->pin, !relay->on_high);
    hal_gpio_write(relay->off_pin, !relay->on_high);
    // then pulse OFF pin
    hal_gpio_write(relay->off_pin, relay->on_high);
    
    // Schedule task to clear both pins after 125ms
    relay->clear_task.handler = relay_clear_handler;
    relay->clear_task.arg = relay;
    hal_tasks_init(&relay->clear_task);
    relay->clear_task_active = 1;
    hal_tasks_schedule(&relay->clear_task, 125);
  }
  
  relay->on = 0;
  
  if (relay->on_change != NULL)
  {
    relay->on_change(relay->callback_param, 0);
  }
}

void relay_toggle(relay_t *relay)
{
  printf("relay_toggle\r\n");
  if (relay->on)
  {
    relay_off(relay);
  } else {
    relay_on(relay);
  }
}
