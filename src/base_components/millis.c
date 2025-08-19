#ifndef _MILLIS_TIMER_H_
#define _MILLIS_TIMER_H_

#include "types.h"
#include "chip_8258/timer.h"

u32 millis_tick_counter = 0;
u32 millis_value        = 0;


void millis_init()
{
  millis_tick_counter = clock_time();
  millis_value        = 0;
}

/**
 * @brief      Advance millis counter
 * @param	   none
 * @return     none
 */
void millis_update()
{
  while (clock_time() - millis_tick_counter >= CLOCK_16M_SYS_TIMER_CLK_1MS)
  {
    millis_tick_counter += CLOCK_16M_SYS_TIMER_CLK_1MS;
    millis_value++;
  }
}


u32 millis()
{
  return(millis_value);
}

u32 seconds()
{
  return(millis_value / 1000);
}

#endif
