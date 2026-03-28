#include "hal/adc.h"

#include <stdbool.h>
#include <stdint.h>

#include "em_gpio.h"
#include "em_iadc.h"
#include "sl_clock_manager.h"

#include "silabs/hal/silabs_gpio_utils.h"

#define IADC_SRC_CLOCK_FREQ_HZ       2000000UL
#define IADC_ADC_CLOCK_FREQ_HZ       1000000UL
#define IADC_RESULT_MAX              0x0FFFUL
#define IADC_INT_REF_MV              1210UL
#define IADC_INTERNAL_DIVIDER        4UL
#define IADC_SINGLE_TIMEOUT_LOOPS    1000000UL

static hal_gpio_pin_t  s_adc_pin   = HAL_INVALID_PIN;
static hal_adc_input_t s_adc_input = HAL_ADC_INPUT_PIN;
static bool            s_adc_ready = false;

typedef struct {
    uint32_t mask;
    uint32_t tristate_value;
    uint32_t adc_value;
} hal_adc_bus_slot_t;

static uint16_t hal_adc_raw_to_mv(uint32_t raw, uint32_t full_scale_mv) {
    return (uint16_t)((raw * full_scale_mv + (IADC_RESULT_MAX / 2U)) /
                      IADC_RESULT_MAX);
}

static void hal_adc_enable_clocks(void) {
    (void)sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_GPIO);
    (void)sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_IADC0);
}

static void hal_adc_shutdown(void) {
    IADC_clearInt(IADC0, IADC_IF_SINGLEDONE);
    IADC_reset(IADC0);
    (void)sl_clock_manager_disable_bus_clock(SL_BUS_CLOCK_IADC0);
}

static bool hal_adc_try_claim_slot(volatile uint32_t *busalloc_reg,
                                   const hal_adc_bus_slot_t *slot) {
    uint32_t field = *busalloc_reg & slot->mask;

    if (field != slot->tristate_value && field != slot->adc_value) {
        return false;
    }

    *busalloc_reg = (*busalloc_reg & ~slot->mask) | slot->adc_value;
    return true;
}

static bool hal_adc_claim_port_bus(GPIO_Port_TypeDef port, uint8_t pin) {
    // We basically need to call hal_adc_try_claim_slot
    // with correct params, but selecting those params depends
    // on port and pin parity, so and this causes this
    // long function.

    volatile uint32_t *busalloc_reg = NULL;
    hal_adc_bus_slot_t slots[2];
    bool is_even = (pin & 1U) == 0U;

    switch (port) {
#if (GPIO_PA_COUNT > 0)
    case gpioPortA:
        busalloc_reg = &GPIO->ABUSALLOC;
        if (is_even) {
            slots[0] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_ABUSALLOC_AEVEN0_MASK,
                .tristate_value = GPIO_ABUSALLOC_AEVEN0_TRISTATE,
                .adc_value      = GPIO_ABUSALLOC_AEVEN0_ADC0,
            };
            slots[1] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_ABUSALLOC_AEVEN1_MASK,
                .tristate_value = GPIO_ABUSALLOC_AEVEN1_TRISTATE,
                .adc_value      = GPIO_ABUSALLOC_AEVEN1_ADC0,
            };
        } else {
            slots[0] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_ABUSALLOC_AODD0_MASK,
                .tristate_value = GPIO_ABUSALLOC_AODD0_TRISTATE,
                .adc_value      = GPIO_ABUSALLOC_AODD0_ADC0,
            };
            slots[1] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_ABUSALLOC_AODD1_MASK,
                .tristate_value = GPIO_ABUSALLOC_AODD1_TRISTATE,
                .adc_value      = GPIO_ABUSALLOC_AODD1_ADC0,
            };
        }
        break;
#endif

#if (GPIO_PB_COUNT > 0)
    case gpioPortB:
        busalloc_reg = &GPIO->BBUSALLOC;
        if (is_even) {
            slots[0] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_BBUSALLOC_BEVEN0_MASK,
                .tristate_value = GPIO_BBUSALLOC_BEVEN0_TRISTATE,
                .adc_value      = GPIO_BBUSALLOC_BEVEN0_ADC0,
            };
            slots[1] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_BBUSALLOC_BEVEN1_MASK,
                .tristate_value = GPIO_BBUSALLOC_BEVEN1_TRISTATE,
                .adc_value      = GPIO_BBUSALLOC_BEVEN1_ADC0,
            };
        } else {
            slots[0] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_BBUSALLOC_BODD0_MASK,
                .tristate_value = GPIO_BBUSALLOC_BODD0_TRISTATE,
                .adc_value      = GPIO_BBUSALLOC_BODD0_ADC0,
            };
            slots[1] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_BBUSALLOC_BODD1_MASK,
                .tristate_value = GPIO_BBUSALLOC_BODD1_TRISTATE,
                .adc_value      = GPIO_BBUSALLOC_BODD1_ADC0,
            };
        }
        break;
#endif

#if (GPIO_PC_COUNT > 0 || GPIO_PD_COUNT > 0)
    case gpioPortC:
    case gpioPortD:
        busalloc_reg = &GPIO->CDBUSALLOC;
        if (is_even) {
            slots[0] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_CDBUSALLOC_CDEVEN0_MASK,
                .tristate_value = GPIO_CDBUSALLOC_CDEVEN0_TRISTATE,
                .adc_value      = GPIO_CDBUSALLOC_CDEVEN0_ADC0,
            };
            slots[1] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_CDBUSALLOC_CDEVEN1_MASK,
                .tristate_value = GPIO_CDBUSALLOC_CDEVEN1_TRISTATE,
                .adc_value      = GPIO_CDBUSALLOC_CDEVEN1_ADC0,
            };
        } else {
            slots[0] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_CDBUSALLOC_CDODD0_MASK,
                .tristate_value = GPIO_CDBUSALLOC_CDODD0_TRISTATE,
                .adc_value      = GPIO_CDBUSALLOC_CDODD0_ADC0,
            };
            slots[1] = (hal_adc_bus_slot_t){
                .mask           = _GPIO_CDBUSALLOC_CDODD1_MASK,
                .tristate_value = GPIO_CDBUSALLOC_CDODD1_TRISTATE,
                .adc_value      = GPIO_CDBUSALLOC_CDODD1_ADC0,
            };
        }
        break;
#endif

    default:
        return false;
    }

    return hal_adc_try_claim_slot(busalloc_reg, &slots[0]) ||
           hal_adc_try_claim_slot(busalloc_reg, &slots[1]);
}

static bool hal_adc_prepare_external_pin(hal_gpio_pin_t pin) {
    GPIO_Port_TypeDef port   = silabs_hal_gpio_port(pin);
    uint8_t           pin_no = silabs_hal_gpio_pin_number(pin);

    (void)sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_GPIO);
    GPIO_PinModeSet(port, pin_no, gpioModeDisabled, 0);
    return hal_adc_claim_port_bus(port, pin_no);
}

static bool hal_adc_configure_single(IADC_PosInput_t pos_input,
                                     IADC_CfgReference_t reference,
                                     uint16_t reference_mv) {
    IADC_Init_t        init        = IADC_INIT_DEFAULT;
    IADC_AllConfigs_t  all_configs = IADC_ALLCONFIGS_DEFAULT;
    IADC_InitSingle_t  init_single = IADC_INITSINGLE_DEFAULT;
    IADC_SingleInput_t input       = IADC_SINGLEINPUT_DEFAULT;

    hal_adc_enable_clocks();

    init.warmup         = iadcWarmupNormal;
    init.srcClkPrescale =
        IADC_calcSrcClkPrescale(IADC0, IADC_SRC_CLOCK_FREQ_HZ, 0);

    all_configs.configs[0].reference      = reference;
    all_configs.configs[0].vRef           = reference_mv;
    all_configs.configs[0].osrHighSpeed   = iadcCfgOsrHighSpeed2x;
    all_configs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(
        IADC0, IADC_ADC_CLOCK_FREQ_HZ, 0, iadcCfgModeNormal,
        init.srcClkPrescale);

    input.posInput = pos_input;

    IADC_reset(IADC0);
    IADC_init(IADC0, &init, &all_configs);
    IADC_initSingle(IADC0, &init_single, &input);

    return true;
}

static uint16_t hal_adc_read_single_raw(IADC_PosInput_t pos_input,
                                        IADC_CfgReference_t reference,
                                        uint16_t reference_mv) {
    IADC_Result_t result;
    uint32_t      timeout = IADC_SINGLE_TIMEOUT_LOOPS;

    if (!hal_adc_configure_single(pos_input, reference, reference_mv)) {
        return 0;
    }

    IADC_clearInt(IADC0, IADC_IF_SINGLEDONE);
    IADC_command(IADC0, iadcCmdStartSingle);

    while (((IADC_getInt(IADC0) & IADC_IF_SINGLEDONE) == 0U) &&
           (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        hal_adc_shutdown();
        return 0;
    }

    result = IADC_readSingleResult(IADC0);

    hal_adc_shutdown();

    return (uint16_t)result.data;
}

static uint16_t hal_adc_read_supply_mv(void) {
    uint16_t raw;

    // Battery-powered Silabs boards expose the supply rail through the
    // internal divided channel, so we do not rely on the config pin here.
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_7) || \
    defined(_SILICON_LABS_32B_SERIES_2_CONFIG_9)
    raw = hal_adc_read_single_raw(iadcPosInputVbat, iadcCfgReferenceInt1V2,
                                  IADC_INT_REF_MV);
#else
    raw = hal_adc_read_single_raw(iadcPosInputAvdd, iadcCfgReferenceInt1V2,
                                  IADC_INT_REF_MV);
#endif

    return hal_adc_raw_to_mv(raw, IADC_INT_REF_MV * IADC_INTERNAL_DIVIDER);
}

static uint16_t hal_adc_read_pin_mv(hal_gpio_pin_t pin) {
    uint16_t avdd_mv;
    uint16_t raw;

    if (pin == HAL_INVALID_PIN || !hal_adc_prepare_external_pin(pin)) {
        return 0;
    }

    avdd_mv = hal_adc_read_supply_mv();
    if (avdd_mv == 0U) {
        return 0;
    }

    raw = hal_adc_read_single_raw(
        IADC_portPinToPosInput(silabs_hal_gpio_port(pin),
                               silabs_hal_gpio_pin_number(pin)),
        iadcCfgReferenceVddx, avdd_mv);

    return hal_adc_raw_to_mv(raw, avdd_mv);
}

void hal_adc_init(hal_adc_input_t input, hal_gpio_pin_t pin) {
    s_adc_input = input;
    s_adc_pin   = pin;
    s_adc_ready = true;
}

uint16_t hal_adc_read_mv() {
    if (!s_adc_ready) {
        return 0;
    }

    if (s_adc_input == HAL_ADC_INPUT_VBAT) {
        return hal_adc_read_supply_mv();
    }

    return hal_adc_read_pin_mv(s_adc_pin);
}
