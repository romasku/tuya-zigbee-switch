#ifndef _HAL_ADC_H_
#define _HAL_ADC_H_

#include "hal/gpio.h"
#include <stdint.h>

typedef enum {
    HAL_ADC_INPUT_PIN,  // ADC from external input
    HAL_ADC_INPUT_VBAT, // ADC from internal voltage bus
} hal_adc_input_t;

void hal_adc_init(hal_adc_input_t input, hal_gpio_pin_t pin);

uint16_t hal_adc_read_mv();

#endif
