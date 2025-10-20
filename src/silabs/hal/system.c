#include "hal/system.h"
#include <stdint.h>
#include <stdbool.h>
#include "micro-common.h"

void hal_system_reset(void) {
    halReboot();
}