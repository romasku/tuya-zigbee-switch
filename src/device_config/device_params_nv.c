#include "device_params_nv.h"
#include "hal/nvm.h"
#include "nvm_items.h"

uint8_t g_multi_press_reset_count = 10;

void device_params_load_from_nv(void) {
    uint8_t value;
    hal_nvm_status_t st =
        hal_nvm_read(NV_ITEM_MULTI_PRESS_RESET_COUNT, sizeof(value),
                     (uint8_t *)&value);

    if (st == HAL_NVM_SUCCESS) {
        g_multi_press_reset_count = value;
    }
}

void device_params_set_multi_press_reset_count(uint8_t value) {
    g_multi_press_reset_count = value;
    hal_nvm_write(NV_ITEM_MULTI_PRESS_RESET_COUNT, sizeof(value),
                  (uint8_t *)&value);
}
