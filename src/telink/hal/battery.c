#include "hal/battery.h"

#ifdef BATTERY_POWERED

#include "tl_common.h"
#include "drivers/drv_adc.h"

#define BATTERY_ADC_PIN    GPIO_PC5

static bool battery_adc_initialized = false;

// Forward declaration
static void battery_adc_init(void);

void hal_battery_reinit_after_retention(void) {
    // After deep retention, ADC hardware needs re-initialization
    battery_adc_initialized = false;
    battery_adc_init();
}

static void battery_adc_init(void) {
    if (battery_adc_initialized) {
        return;
    }
    drv_adc_init();
    drv_adc_mode_pin_set(DRV_ADC_VBAT_MODE, BATTERY_ADC_PIN);
    battery_adc_initialized = true;
}

static uint16_t battery_adc_read_mv(void) {
    battery_adc_init();
    drv_adc_enable(true);
    sleep_us(100);
    uint16_t voltage_mv = drv_get_adc_data();
    drv_adc_enable(false);
    return voltage_mv;
}

uint16_t hal_battery_get_voltage_mv(void) {
    uint16_t mv = battery_adc_read_mv();

    return mv;
}

#endif // BATTERY_POWERED
