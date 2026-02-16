#include "hal/battery.h"

#ifdef BATTERY_POWERED

#include "tl_common.h"
#include "drivers/drv_adc.h"

// Min and max battery voltages in mV
// Used to calculate battery percentage
// For CR2032 min is 2.0V and max is 3.0V
// For CR2450 min is 2.0V and max is 3.0V
// For CR2430 min is 2.0V and max is 3.0V
#define BATTERY_VOLTAGE_MIN_MV 2000
#define BATTERY_VOLTAGE_MAX_MV 3000

#define BATTERY_ADC_PIN         GPIO_PC5

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

uint8_t hal_battery_get_percentage(void) {
    uint16_t voltage_mv = hal_battery_get_voltage_mv();

    uint8_t percentage;
    if (voltage_mv >= BATTERY_VOLTAGE_MAX_MV) {
        percentage = 100;
    } else if (voltage_mv <= BATTERY_VOLTAGE_MIN_MV) {
        percentage = 0;
    } else {
        uint32_t range = BATTERY_VOLTAGE_MAX_MV - BATTERY_VOLTAGE_MIN_MV;
        uint32_t level = voltage_mv - BATTERY_VOLTAGE_MIN_MV;
        percentage = (uint8_t)((level * 100) / range);
    }

    printf("[%d] [BATTERY] %d mV -> %d%%\r\n", clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS, voltage_mv, percentage);
    return percentage;
}

uint16_t hal_battery_get_voltage_mv(void) {
    uint16_t mv = battery_adc_read_mv();
    return mv;
}

#endif // BATTERY_POWERED
