#include "em_assert.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_gpio.h"
#include "gpiointerrupt.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zigbee_app_framework_event.h"

#include "hal/gpio.h"
#include <stdio.h>

// Get container structure from embedded member pointer
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr) - offsetof(type, member)))

// ------ Encoding helpers ------
// hal_gpio_pin_t: upper byte = port index (A=0, B=1, ...), lower byte = pin
// [0..15]
#define HAL_GPIO_PIN_NUM(g) ((uint8_t)((g) & 0xFF))
#define HAL_GPIO_PORT_INDEX(g) ((uint8_t)(((g) >> 8) & 0xFF))

#define LINE_MISSING 0xFF
#define MAX_INT_LINES 16

static inline GPIO_Port_TypeDef hal_port_from_index(uint8_t idx) {
  static const GPIO_Port_TypeDef lut[] = {gpioPortA, gpioPortB, gpioPortC,
                                          gpioPortD};
  EFM_ASSERT(idx < (sizeof(lut) / sizeof(lut[0])));
  return lut[idx];
}

// ------ One-time init guards ------
static bool s_gpio_clock_enabled = false;
static bool s_gpioint_inited = false;

static void hal_gpio_ensure_clock(void) {
  if (!s_gpio_clock_enabled) {
    CMU_ClockEnable(cmuClock_GPIO, true);
    s_gpio_clock_enabled = true;
  }
}

static void hal_gpio_ensure_gpioint(void) {
  if (!s_gpioint_inited) {
    GPIOINT_Init(); // sets up EVEN/ODD IRQs
    s_gpioint_inited = true;
  }
}

// ------ Per-interrupt bookkeeping ------
typedef struct {
  bool in_use;
  hal_gpio_pin_t hal_pin;
  uint8_t pull_dir; // for EM4WU polarity selection
  gpio_callback_t user_cb;
  void *arg;
  sli_zigbee_event_t af_event;
} int_slot_t;

static int_slot_t s_slots[MAX_INT_LINES]; // 16 EXTI lines total

// Allocate a free EXTI line [0..15]; returns 0..15 or LINE_MISSING if none
static uint8_t alloc_int_line(void) {
  for (uint8_t i = 0; i < 16; i++) {
    if (!s_slots[i].in_use) {
      s_slots[i].in_use = true;
      return i;
    }
  }
  return LINE_MISSING;
}

static void free_int_line(uint8_t int_no) {
  if (int_no < MAX_INT_LINES) {
    memset(&s_slots[int_no], 0, sizeof(s_slots[int_no]));
  }
}

// Dispatchers, e.g. IRQ routinues
static void _dispatch_regular(uint8_t intNo, void *ctx) {
  (void)intNo;
  int_slot_t *slot = (int_slot_t *)ctx;
  if (slot) {
    sl_zigbee_af_event_set_active(&slot->af_event);
  }
}

static void _af_event_handler(sl_zigbee_af_event_t *event) {
  // Get int_slot_t from embedded af_event field
  int_slot_t *slot = container_of(event, int_slot_t, af_event);
  if (slot->user_cb) {
    slot->user_cb(slot->hal_pin, slot->arg);
  }
}

// API
void hal_gpio_init(hal_gpio_pin_t gpio_pin, uint8_t is_input,
                   hal_gpio_pull_t pull_direction) {
  hal_gpio_ensure_clock();

  const uint8_t port_idx = HAL_GPIO_PORT_INDEX(gpio_pin);
  const uint8_t pin_num = HAL_GPIO_PIN_NUM(gpio_pin);
  GPIO_Port_TypeDef port = hal_port_from_index(port_idx);

  if (is_input) {
    switch (pull_direction) {
    case HAL_GPIO_PULL_UP:
      GPIO_PinModeSet(port, pin_num, gpioModeInputPull, 1); // DOUT=1 => pull-up
      break;
    case HAL_GPIO_PULL_DOWN:
      GPIO_PinModeSet(port, pin_num, gpioModeInputPull,
                      0); // DOUT=0 => pull-down
      break;
    default:
      GPIO_PinModeSet(port, pin_num, gpioModeInput, 0);
      break;
    }
  } else {
    // Output: push-pull, initial low
    GPIO_PinModeSet(port, pin_num, gpioModePushPull, 0);
  }

  // Optional: store pull direction for interrupt polarity later.
  // We don’t keep a global pin map; polarity is picked at registration time
  // by looking up the slot when the user calls hal_gpio_int_callback.
}

void hal_gpio_set(hal_gpio_pin_t gpio_pin) {
  const uint8_t port_idx = HAL_GPIO_PORT_INDEX(gpio_pin);
  const uint8_t pin_num = HAL_GPIO_PIN_NUM(gpio_pin);
  GPIO_PinOutSet(hal_port_from_index(port_idx), pin_num);
}

void hal_gpio_clear(hal_gpio_pin_t gpio_pin) {
  const uint8_t port_idx = HAL_GPIO_PORT_INDEX(gpio_pin);
  const uint8_t pin_num = HAL_GPIO_PIN_NUM(gpio_pin);
  GPIO_PinOutClear(hal_port_from_index(port_idx), pin_num);
}

uint8_t hal_gpio_read(hal_gpio_pin_t gpio_pin) {
  const uint8_t port_idx = HAL_GPIO_PORT_INDEX(gpio_pin);
  const uint8_t pin_num = HAL_GPIO_PIN_NUM(gpio_pin);
  return GPIO_PinInGet(hal_port_from_index(port_idx), pin_num);
}

// Register an interrupt that also attempts EM4 wake-up.
// - Wakes from EM2/EM3 via normal EXTI (edge-sensitive).
// - Wakes from EM4 if the pin supports EM4WU (level-sensitive).
//   Polarity rule:
//     * If input has pull-up  -> active-low (wake on low level).
//     * If input has pull-down-> active-high (wake on high level).
//     * Otherwise default to rising+falling EXTI and active-low EM4WU.
void hal_gpio_callback(hal_gpio_pin_t gpio_pin, gpio_callback_t callback,
                       void *arg) {
  hal_gpio_ensure_clock();
  hal_gpio_ensure_gpioint();

  const uint8_t port_idx = HAL_GPIO_PORT_INDEX(gpio_pin);
  const uint8_t pin_num = HAL_GPIO_PIN_NUM(gpio_pin);
  GPIO_Port_TypeDef port = hal_port_from_index(port_idx);

  // Allocate a regular EXTI line (for edge interrupts while awake)
  uint8_t line = alloc_int_line();
  EFM_ASSERT(line != LINE_MISSING); // out of lines
  int_slot_t *slot = &s_slots[line];
  slot->hal_pin = gpio_pin;
  slot->user_cb = callback;
  slot->arg = arg;
  sl_zigbee_af_isr_event_init(&slot->af_event, _af_event_handler);

  GPIO_Mode_TypeDef mode = GPIO_PinModeGet(port, pin_num);
  if (mode == gpioModeInputPull) {
    if (GPIO_PinOutGet(port, pin_num)) {
      slot->pull_dir = HAL_GPIO_PULL_UP;
    } else {
      slot->pull_dir = HAL_GPIO_PULL_DOWN;
    }
  } else {
    slot->pull_dir = HAL_GPIO_PULL_NONE;
  }

  // Register regular edge-sensitive callback (both edges)
  unsigned int reg_int = GPIOINT_CallbackRegisterExt(
      line, (GPIOINT_IrqCallbackPtrExt_t)_dispatch_regular, slot);
  EFM_ASSERT(reg_int != INTERRUPT_UNAVAILABLE);
  GPIO_ExtIntConfig(port, pin_num, line, true, true, true);
}

// (Optional) helper to unregister an interrupt if you add
// hal_gpio_int_disable() later
void hal_gpio_unreg_callback(hal_gpio_pin_t gpio_pin) {
  const uint8_t port_idx = HAL_GPIO_PORT_INDEX(gpio_pin);
  const uint8_t pin_num = HAL_GPIO_PIN_NUM(gpio_pin);
  GPIO_Port_TypeDef port = hal_port_from_index(port_idx);

  uint8_t int_no = LINE_MISSING;
  for (uint8_t i = 0; i < MAX_INT_LINES; i++) {
    if (s_slots[i].in_use && s_slots[i].hal_pin == gpio_pin) {
      int_no = i;
    }
  }

  if (int_no != LINE_MISSING) {
    GPIO_ExtIntConfig(port, pin_num, int_no, false, false, false);
    GPIOINT_CallbackUnRegister(int_no);
    free_int_line(int_no);
  }
}

hal_gpio_pin_t hal_gpio_parse_pin(const char *s) {
  if (!s || s[0] < 'A' || s[0] > 'D' || s[1] < '0' || s[1] > '8')
    return HAL_INVALID_PIN;

  static const uint8_t ports[] = {gpioPortA, gpioPortB, gpioPortC, gpioPortD};
  return (hal_gpio_pin_t)((ports[s[0] - 'A'] << 8) | (uint8_t)(s[1] - '0'));
}

hal_gpio_pull_t hal_gpio_parse_pull(const char *pull_str) {
  if (pull_str[0] == 'u') {
    return HAL_GPIO_PULL_UP;
  }
  if (pull_str[0] == 'd') {
    return HAL_GPIO_PULL_DOWN;
  }
  if (pull_str[0] == 'f') {
    return HAL_GPIO_PULL_NONE;
  }
  return HAL_GPIO_PULL_INVALID;
}
