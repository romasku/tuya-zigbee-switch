#include "hal/gpio.h"
#include "hal/tasks.h"
#include "tl_common.h"
#include <stdint.h>
#include <string.h>

// Maximum number of GPIO interrupts we can handle
#define MAX_GPIO_CALLBACKS 16

static inline uint32_t pin_to_mask(hal_gpio_pin_t pin) {
  uint32_t pin_one_hot = (pin & 0xFF);
  uint32_t port = (pin >> 8) & 0x07;
  return (pin_one_hot << (port * 8));
}

typedef struct {
  hal_gpio_pin_t gpio_pin;
  gpio_callback_t callback;
  void *arg;
} gpio_callback_info_t;

static gpio_callback_info_t gpio_callbacks[MAX_GPIO_CALLBACKS];
static hal_task_t gpio_dispatch_task;

static volatile uint32_t prev_gpio_state = 0;
static volatile uint32_t pending_gpio_mask = 0;

static void gpio_dispatch_handler(void *arg) {
  drv_disable_irq();
  uint32_t current_pending = pending_gpio_mask;
  pending_gpio_mask = 0;
  drv_enable_irq();

  for (gpio_callback_info_t *info = gpio_callbacks;
       info < gpio_callbacks + MAX_GPIO_CALLBACKS; info++) {
    if (info->gpio_pin == HAL_INVALID_PIN) {
      continue;
    }
    if (current_pending & pin_to_mask(info->gpio_pin)) {
      info->callback(info->gpio_pin, info->arg);
    }
  }
}

static void gpio_dispatch_init(void) {
  if (gpio_dispatch_task.handler == NULL) {
    gpio_dispatch_task.handler = gpio_dispatch_handler;
    gpio_dispatch_task.arg = NULL;
    hal_tasks_init(&gpio_dispatch_task);
    for (int i = 0; i < MAX_GPIO_CALLBACKS; ++i) {
      gpio_callbacks[i].gpio_pin = HAL_INVALID_PIN;
      gpio_callbacks[i].callback = NULL;
      gpio_callbacks[i].arg = NULL;
    }
  }
}

static void gpio_isr_callback(void) {
  uint32_t current_state = 0;
  drv_gpio_read_all((uint8_t *)&current_state);
  uint32_t changed = current_state ^ prev_gpio_state;

  pending_gpio_mask |= changed;

  if (changed) {
    hal_tasks_unschedule(&gpio_dispatch_task);
    hal_tasks_schedule(&gpio_dispatch_task, 0);
  }

  for (gpio_callback_info_t *info = gpio_callbacks;
       info < gpio_callbacks + MAX_GPIO_CALLBACKS; info++) {
    if (info->gpio_pin == HAL_INVALID_PIN ||
        !(changed & pin_to_mask(info->gpio_pin))) {
      continue;
    }
    drv_gpio_irq_set((u32)info->gpio_pin,
                     (current_state & pin_to_mask(info->gpio_pin))
                         ? GPIO_FALLING_EDGE
                         : GPIO_RISING_EDGE);
    // cpu_set_gpio_wakeup(info->gpio_pin,
    //                     (current_state & pin_to_mask(info->gpio_pin)) ? 0 :
    //                     1, 1);
  }

  prev_gpio_state = current_state;
}

void hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback,
                       void *arg) {
  gpio_dispatch_init();

  gpio_callback_info_t *slot = NULL;

  for (gpio_callback_info_t *info = gpio_callbacks;
       info < gpio_callbacks + MAX_GPIO_CALLBACKS; info++) {
    if (info->gpio_pin == HAL_INVALID_PIN || info->gpio_pin == gpio_pin) {
      slot = info;
      break;
    }
  }

  if (slot == NULL) {
    return;
  }

  slot->gpio_pin = gpio_pin;
  slot->callback = callback;
  slot->arg = arg;

  drv_gpioPoll_e edge;
  drv_disable_irq();
  if (hal_gpio_read(gpio_pin)) {
    prev_gpio_state |= pin_to_mask(gpio_pin);
    edge = GPIO_FALLING_EDGE;
  } else {

    prev_gpio_state &= ~pin_to_mask(gpio_pin);
    edge = GPIO_RISING_EDGE;
  }
  drv_enable_irq();

  int result = drv_gpio_irq_config(GPIO_IRQ_MODE, (u32)gpio_pin, edge,
                                   gpio_isr_callback);
  if (result != 0) {
    printf("Failed to configure GPIO interrupt: %d\r\n", result);
  }
  drv_gpio_irq_en((u32)gpio_pin);
}

void hal_gpio_unreg_callback(hal_gpio_pin_t gpio_pin) {

  for (uint8_t i = 0; i < MAX_GPIO_CALLBACKS; i++) {
    if (gpio_callbacks[i].gpio_pin == gpio_pin) {
      drv_gpio_irq_dis((u32)gpio_pin);

      gpio_callbacks[i].gpio_pin = HAL_INVALID_PIN;

      break;
    }
  }
}

void telink_gpio_hal_setup_wake_ups() {
  for (gpio_callback_info_t *info = gpio_callbacks;
       info < gpio_callbacks + MAX_GPIO_CALLBACKS; info++) {
    if (info->gpio_pin == HAL_INVALID_PIN) {
      continue;
    }
    cpu_set_gpio_wakeup(info->gpio_pin, (hal_gpio_read(info->gpio_pin)) ? 0 : 1,
                        1);
  }
}