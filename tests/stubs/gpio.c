#include "gpio.h"

#define GPIO_MAX 0x500

u8 gpio_state[GPIO_MAX];

void drv_gpio_write(u32 pin, u8 level) {
    if (pin < GPIO_MAX) gpio_state[pin] = level; 
}

u8 drv_gpio_read(u32 pin) { 
    return (pin < GPIO_MAX) ? gpio_state[pin] : 0; 
}

