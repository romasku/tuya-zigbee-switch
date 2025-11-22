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

static void ensure_valid_edges() {
  uint32_t current_state = 0;
  uint32_t prev_state = 0;
  drv_gpio_read_all((uint8_t *)&current_state);
  do {
    prev_state = current_state;
    for (gpio_callback_info_t *info = gpio_callbacks;
         info < gpio_callbacks + MAX_GPIO_CALLBACKS; info++) {
      if (info->gpio_pin == HAL_INVALID_PIN) {
        continue;
      }
      drv_gpio_irq_set((u32)info->gpio_pin,
                       (current_state & pin_to_mask(info->gpio_pin))
                           ? GPIO_FALLING_EDGE
                           : GPIO_RISING_EDGE);
    }
    drv_gpio_read_all((uint8_t *)&current_state);
  } while (current_state != prev_state);
}

static void gpio_dispatch_handler(void *arg) {
  ensure_valid_edges();

  for (gpio_callback_info_t *info = gpio_callbacks;
       info < gpio_callbacks + MAX_GPIO_CALLBACKS; info++) {
    if (info->gpio_pin == HAL_INVALID_PIN) {
      continue;
    }
    info->callback(info->gpio_pin, info->arg);
  }
}

static void gpio_isr_callback(void) {
  hal_tasks_unschedule(&gpio_dispatch_task);
  hal_tasks_schedule(&gpio_dispatch_task, 0);
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

static bool is_isr_initialized = false;

static void init_isr() {
  if (is_isr_initialized) {
    return;
  }
  // PIN and EDGE doesn't matter here, SDK selects callbacks only
  // by mode
  int result = drv_gpio_irq_config(GPIO_IRQ_MODE, 0, 0, gpio_isr_callback);
  if (result != 0) {
    printf("Failed to configure GPIO interrupt: %d\r\n", result);
  } else {
    is_isr_initialized = true;
    printf("GPIO interrupt configured\r\n");
  }
}

void hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback,
                       void *arg) {
  gpio_dispatch_init();
  init_isr();

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

  ensure_valid_edges();

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