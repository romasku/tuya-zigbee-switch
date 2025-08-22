#ifndef MILLIS_STUB_H
#define MILLIS_STUB_H

#include "types.h"

u32 millis(void);
void millis_set(u32 value);
void millis_advance(u32 ms);
void millis_init(void);
void millis_update(void);

#endif