#include "timer_stubs.h"
#include <stddef.h>

// Mock timer event storage
static ev_timer_event_t mock_timer_events[10];
static int mock_timer_count = 0;

ev_timer_event_t* TL_ZB_TIMER_SCHEDULE(timer_callback_t callback, void* arg, unsigned int interval_ms)
{
  if (mock_timer_count >= 10) {
    return NULL;
  }
  
  ev_timer_event_t* evt = &mock_timer_events[mock_timer_count];
  evt->callback = callback;
  evt->arg = arg;
  evt->interval_ms = interval_ms;
  evt->active = 1;
  
  mock_timer_count++;
  return evt;
}

void TL_ZB_TIMER_CANCEL(ev_timer_event_t** evt)
{
  if (evt && *evt) {
    (*evt)->active = 0;
    *evt = NULL;
  }
}

// Reset mock state for tests
void timer_stubs_reset(void)
{
  mock_timer_count = 0;
  for (int i = 0; i < 10; i++) {
    mock_timer_events[i].active = 0;
  }
}