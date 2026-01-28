#include "hal/timer.h"
#include "sl_sleeptimer.h"


uint32_t hal_millis() {
    return sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count());
}
