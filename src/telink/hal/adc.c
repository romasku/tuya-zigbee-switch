#include "hal/adc.h"
#pragma pack(push, 1)
#include "tl_common.h"
#include "drivers/drv_adc.h"
#pragma pack(pop)

static hal_gpio_pin_t  adc_pin = HAL_INVALID_PIN;
static hal_adc_input_t adc_input;
// After deep retention, we need to re-initialize ADC hardware before reading,
// So this flag should go to NOT retained section.
static _attribute_custom_data_ bool adc_initialized = false;


void hal_adc_init(hal_adc_input_t input, hal_gpio_pin_t pin) {
    drv_adc_init();
    if (input == HAL_ADC_INPUT_VBAT) {
        drv_adc_mode_pin_set(DRV_ADC_VBAT_MODE, (GPIO_PinTypeDef)pin);
    } else {
        drv_adc_mode_pin_set(DRV_ADC_BASE_MODE, (GPIO_PinTypeDef)pin);
    }
    adc_pin         = pin;
    adc_input       = input;
    adc_initialized = true;
}

uint16_t hal_adc_read_mv() {
    if (adc_pin == HAL_INVALID_PIN) {
        return 0; // Not initialized
    }
    if (!adc_initialized) {
        // After deep retention, ADC hardware needs re-initialization
        hal_adc_init(adc_input, adc_pin);
    }
    drv_adc_enable(true);
    sleep_us(100);  // TODO do actually need this?
    uint16_t voltage_mv = drv_get_adc_data();
    drv_adc_enable(false);
    printf("Battery voltage (mV): %d\r\n", voltage_mv);
    return voltage_mv;
}
