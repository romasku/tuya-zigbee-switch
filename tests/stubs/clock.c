#include "clock.h"

static u32 fake_time = 0;

u32 clock_time(void) { return fake_time; }
void stub_clock_set(u32 t){ fake_time = t; }
void stub_clock_advance(u32 delta){ fake_time += delta; }