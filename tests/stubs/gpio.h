
#include "types.h"

#define GPIO_MAX 0x500

extern u8 gpio_state[GPIO_MAX];

void drv_gpio_write(u32 pin, u8 level);
u8 drv_gpio_read(u32 pin);
void gpio_write(u32 pin, u8 level);