#ifndef HAL_PRINTF_H
#define HAL_PRINTF_H

#ifdef HAL_TELINK

extern int tl_printf(const char *format, ...);

#define printf    tl_printf

#elif defined(HAL_SILABS)

#include "stdio.h"

#else
#include <stdio.h>
#endif

#endif
