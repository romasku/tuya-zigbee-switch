#pragma once

#include "types.h"

// Timer callback function type
typedef int (*timer_callback_t)(void* arg);

// Timer event structure
typedef struct {
  timer_callback_t callback;
  void* arg;
  unsigned int interval_ms;
  unsigned char active;
} ev_timer_event_t;

// Timer API functions
ev_timer_event_t* TL_ZB_TIMER_SCHEDULE(timer_callback_t callback, void* arg, unsigned int interval_ms);
void TL_ZB_TIMER_CANCEL(ev_timer_event_t** evt);

// Test utility
void timer_stubs_reset(void);