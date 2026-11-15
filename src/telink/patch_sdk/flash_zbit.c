#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)

/*
 * Flash writes on ZBIT flash chips (JEDEC id 0x13325E / 0x14325E, found in 512 kB modules).
 *
 * For these chips the SDK measures the supply voltage with the ADC before every program and erase
 * command (flash_mspi_write_ram in platform/chip_8258/flash.c) and silently drops the command when
 * the reading is not above 2.2 V or fluctuates. On the TLSR8258 the supply can only be measured
 * through a free GPIO (VBAT mode); this firmware has VOLTAGE_DETECT_ADC_PIN = 0, so the reading is
 * meaningless and most writes were dropped: the Zigbee stack's data (network key, bindings, frame
 * counter) never reached the flash. The device lost its network at every restart.
 *
 * sdk.mk compiles flash.c with adc_get_result_with_fluct renamed to this function.
 * Mains powered devices: the supply is regulated, report it as good.
 * Battery powered devices: the ADC is set up on the battery pin, keep the SDK's check.
 */
unsigned char g_zbit_voltage_gate = 0; // set to 1 when the ADC measures the battery

_attribute_ram_code_ unsigned int zbit_supply_mv(unsigned int *fluct) {
    if (g_zbit_voltage_gate) {
        return adc_get_result_with_fluct(fluct);
    }
    *fluct = 0;
    return 3300;
}
