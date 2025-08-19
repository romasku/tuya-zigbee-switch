#pragma once

#include "types.h"

void drv_gpio_write(u32 pin, bool value);
bool drv_gpio_read(u32 pin);
