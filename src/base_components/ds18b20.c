#include "ds18b20.h"

#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "zigbee/consts.h"

#include <string.h>

#if defined(HAL_TELINK)
#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)
#endif

#define DS18B20_FAMILY             0x28
#define DS2438_FAMILY              0x26
#define DS18B20_NV_MAGIC           0xB6
#define DS18B20_MISS_LIMIT         3
#define DS18B20_CMD_SEARCH         0xF0
#define DS18B20_CMD_MATCH          0x55
#define DS18B20_CMD_SKIP           0xCC
#define DS18B20_CMD_CONVERT        0x44
#define DS18B20_CMD_READ_SCRATCH   0xBE
#define DS2438_CMD_CONVERT_V       0xB4
#define DS2438_CMD_WRITE_SCRATCH   0x4E
#define DS2438_AD_VAD              0x08

typedef struct {
    uint8_t magic;
    uint8_t occupied;
    uint8_t rom[DS18B20_MAX_SENSORS][8];
} ds18b20_nv_t;

static hal_gpio_pin_t bus_pin = HAL_INVALID_PIN;
static ds18b20_nv_t   slots;
static uint8_t        miss_count[DS18B20_MAX_SENSORS];
static uint8_t        seen_mask;

static uint8_t onewire_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

#if defined(HAL_TELINK) || defined(HAL_SILABS)

static void ow_delay_us(uint32_t us) {
#if defined(HAL_TELINK)
    sleep_us(us);
#else
    volatile uint32_t count = us * 8;
    while (count--) {}
#endif
}

static uint32_t ow_irq_save(void) {
#if defined(HAL_TELINK)
    return irq_disable();
#else
    return 0;
#endif
}

static void ow_irq_restore(uint32_t state) {
#if defined(HAL_TELINK)
    irq_restore(state);
#else
    (void)state;
#endif
}

static void ow_drive_low(void) {
#if defined(HAL_TELINK)
    GPIO_PinTypeDef pin = (GPIO_PinTypeDef)bus_pin;
    gpio_write(pin, 0);
    gpio_set_output_en(pin, 1);
    gpio_set_input_en(pin, 0);
#else
    hal_gpio_init(bus_pin, 0, HAL_GPIO_PULL_NONE);
    hal_gpio_clear(bus_pin);
#endif
}

static void ow_release(void) {
#if defined(HAL_TELINK)
    GPIO_PinTypeDef pin = (GPIO_PinTypeDef)bus_pin;
    gpio_set_output_en(pin, 0);
    gpio_set_input_en(pin, 1);
    gpio_setup_up_down_resistor(pin, PM_PIN_PULLUP_10K);
#else
    hal_gpio_init(bus_pin, 1, HAL_GPIO_PULL_UP);
#endif
}

static uint8_t ow_sample(void) {
#if defined(HAL_TELINK)
    return gpio_read((GPIO_PinTypeDef)bus_pin) ? 1 : 0;
#else
    return hal_gpio_read(bus_pin);
#endif
}

static uint8_t ow_reset(void) {
    ow_drive_low();
    ow_delay_us(480);
    ow_release();
    ow_delay_us(70);
    uint8_t present = ow_sample() == 0;
    ow_delay_us(410);
    return present;
}

static void ow_write_bit(uint8_t bit) {
    uint32_t state = ow_irq_save();
    ow_drive_low();
    if (bit) {
        ow_delay_us(6);
        ow_release();
        ow_irq_restore(state);
        ow_delay_us(64);
    } else {
        ow_delay_us(60);
        ow_release();
        ow_irq_restore(state);
        ow_delay_us(10);
    }
}

static uint8_t ow_read_bit(void) {
    uint32_t state = ow_irq_save();
    ow_drive_low();
    ow_delay_us(6);
    ow_release();
    ow_delay_us(9);
    uint8_t bit = ow_sample();
    ow_irq_restore(state);
    ow_delay_us(55);
    return bit;
}

static void ow_write_byte(uint8_t value) {
    for (uint8_t bit = 0; bit < 8; bit++) {
        ow_write_bit(value & 0x01);
        value >>= 1;
    }
}

static uint8_t ow_read_byte(void) {
    uint8_t value = 0;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (ow_read_bit()) {
            value |= (uint8_t)(1u << bit);
        }
    }
    return value;
}

/*
 * Dallas search ROM. last_discrepancy == 0 and last_device set means the
 * previous call returned the final device.
 */
static uint8_t ow_search_next(uint8_t rom[8], uint8_t *last_discrepancy,
                              uint8_t *last_device) {
    if (*last_device) {
        return 0;
    }
    if (!ow_reset()) {
        *last_discrepancy = 0;
        *last_device      = 1;
        return 0;
    }

    ow_write_byte(DS18B20_CMD_SEARCH);

    uint8_t last_zero   = 0;
    uint8_t rom_byte    = 0;
    uint8_t rom_mask    = 1;
    uint8_t bit_number  = 1;

    for (uint8_t i = 0; i < 64; i++) {
        uint8_t bit     = ow_read_bit();
        uint8_t complement = ow_read_bit();
        uint8_t direction;

        if (bit && complement) {
            *last_discrepancy = 0;
            *last_device      = 1;
            return 0;
        }
        if (bit != complement) {
            direction = bit;
        } else {
            if (bit_number < *last_discrepancy) {
                direction = (rom[rom_byte] & rom_mask) ? 1 : 0;
            } else if (bit_number == *last_discrepancy) {
                direction = 1;
            } else {
                direction = 0;
            }
            if (!direction) {
                last_zero = bit_number;
            }
        }

        if (direction) {
            rom[rom_byte] |= rom_mask;
        } else {
            rom[rom_byte] &= (uint8_t)~rom_mask;
        }
        ow_write_bit(direction);

        bit_number++;
        rom_mask <<= 1;
        if (rom_mask == 0) {
            rom_byte++;
            rom_mask = 1;
        }
    }

    *last_discrepancy = last_zero;
    if (*last_discrepancy == 0) {
        *last_device = 1;
    }
    return onewire_crc8(rom, 7) == rom[7];
}

static int slot_for_rom(const uint8_t rom[8]) {
    for (uint8_t i = 0; i < DS18B20_MAX_SENSORS; i++) {
        if ((slots.occupied & (1u << i)) &&
            memcmp(slots.rom[i], rom, 8) == 0) {
            return i;
        }
    }
    return -1;
}

static int first_free_slot(void) {
    for (uint8_t i = 0; i < DS18B20_MAX_SENSORS; i++) {
        if ((slots.occupied & (1u << i)) == 0) {
            return i;
        }
    }
    return -1;
}

static void save_slots(void) {
    slots.magic = DS18B20_NV_MAGIC;
    hal_nvm_status_t status =
        hal_nvm_write(NV_ITEM_DS18B20_SLOTS, sizeof(slots), (uint8_t *)&slots);
    if (status != HAL_NVM_SUCCESS) {
        printf("DS18B20 slot save failed: %d\r\n", status);
    }
}

static void load_slots(void) {
    memset(&slots, 0, sizeof(slots));
    memset(miss_count, 0, sizeof(miss_count));
    ds18b20_nv_t stored;
    if (hal_nvm_read(NV_ITEM_DS18B20_SLOTS, sizeof(stored),
                     (uint8_t *)&stored) == HAL_NVM_SUCCESS &&
        stored.magic == DS18B20_NV_MAGIC) {
        slots = stored;
    }
}

static void reconcile_found(const uint8_t rom[8]) {
    int slot = slot_for_rom(rom);
    if (slot < 0) {
        slot = first_free_slot();
        if (slot < 0) {
            printf("DS18B20 bus full, ignoring extra sensor\r\n");
            return;
        }
        memcpy(slots.rom[slot], rom, 8);
        slots.occupied |= (uint8_t)(1u << slot);
        save_slots();
        printf("1-Wire slot %d assigned\r\n", slot);
    }
    miss_count[slot] = 0;
    seen_mask |= (uint8_t)(1u << slot);
}

static uint8_t read_scratchpad(const uint8_t rom[8], int16_t *raw) {
    if (!ow_reset()) {
        return 0;
    }
    ow_write_byte(DS18B20_CMD_MATCH);
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_byte(rom[i]);
    }
    ow_write_byte(DS18B20_CMD_READ_SCRATCH);

    uint8_t scratch[9];
    for (uint8_t i = 0; i < 9; i++) {
        scratch[i] = ow_read_byte();
    }
    if (onewire_crc8(scratch, 8) != scratch[8]) {
        return 0;
    }
    *raw = (int16_t)((uint16_t)scratch[0] | ((uint16_t)scratch[1] << 8));
    return 1;
}

static uint8_t ow_match(const uint8_t rom[8]) {
    if (!ow_reset()) {
        return 0;
    }
    ow_write_byte(DS18B20_CMD_MATCH);
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_byte(rom[i]);
    }
    return 1;
}

static uint8_t ds2438_read_page(const uint8_t rom[8], uint8_t page,
                                uint8_t data[8]) {
    if (!ow_match(rom)) {
        return 0;
    }
    ow_write_byte(DS18B20_CMD_READ_SCRATCH);
    ow_write_byte(page);

    uint8_t scratch[9];
    for (uint8_t i = 0; i < 9; i++) {
        scratch[i] = ow_read_byte();
    }
    if (onewire_crc8(scratch, 8) != scratch[8]) {
        return 0;
    }
    memcpy(data, scratch, 8);
    return 1;
}

static uint8_t ds2438_write_page(const uint8_t rom[8], uint8_t page,
                                 const uint8_t data[8]) {
    if (!ow_match(rom)) {
        return 0;
    }
    ow_write_byte(DS2438_CMD_WRITE_SCRATCH);
    ow_write_byte(page);
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_byte(data[i]);
    }
    return 1;
}

/* VAD when select_vad is 1, otherwise the chip supply (VDD). Result is millivolts. */
static uint8_t ds2438_read_voltage(const uint8_t rom[8], uint8_t select_vad,
                                   uint16_t *millivolts) {
    uint8_t page[8];
    if (!ds2438_read_page(rom, 0, page)) {
        return 0;
    }
    page[0] = (uint8_t)((page[0] & (uint8_t)~DS2438_AD_VAD) |
                        (select_vad ? DS2438_AD_VAD : 0));
    if (!ds2438_write_page(rom, 0, page)) {
        return 0;
    }
    if (!ow_match(rom)) {
        return 0;
    }
    ow_write_byte(DS2438_CMD_CONVERT_V);
    ow_delay_us(10000);
    if (!ds2438_read_page(rom, 0, page)) {
        return 0;
    }
    uint16_t raw = (uint16_t)(page[3] | (page[4] << 8)) & 0x03FF;
    *millivolts = (uint16_t)(raw * 10);
    return 1;
}

#else

static void load_slots(void) {
    memset(&slots, 0, sizeof(slots));
}

#endif

void ds18b20_init(hal_gpio_pin_t pin) {
    bus_pin = pin;
    load_slots();
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    hal_gpio_init(pin, 1, HAL_GPIO_PULL_UP);
#else
    (void)pin;
#endif
}

uint8_t ds18b20_is_configured(void) {
    return bus_pin != HAL_INVALID_PIN;
}

void ds18b20_start_conversion(void) {
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    seen_mask = 0;
    uint8_t rom[8];
    uint8_t last_discrepancy = 0;
    uint8_t last_device      = 0;
    memset(rom, 0, sizeof(rom));

    while (!last_device) {
        if (!ow_search_next(rom, &last_discrepancy, &last_device)) {
            if (last_device) {
                break;
            }
            continue;
        }
        if (rom[0] == DS18B20_FAMILY || rom[0] == DS2438_FAMILY) {
            reconcile_found(rom);
        }
    }

    for (uint8_t i = 0; i < DS18B20_MAX_SENSORS; i++) {
        if ((slots.occupied & (1u << i)) == 0 || (seen_mask & (1u << i))) {
            continue;
        }
        if (miss_count[i] < 255) {
            miss_count[i]++;
        }
        if (miss_count[i] >= DS18B20_MISS_LIMIT) {
            slots.occupied &= (uint8_t)~(1u << i);
            memset(slots.rom[i], 0, 8);
            miss_count[i] = 0;
            save_slots();
            printf("1-Wire slot %d released\r\n", i);
        }
    }

    if (seen_mask == 0 || !ow_reset()) {
        return;
    }
    ow_write_byte(DS18B20_CMD_SKIP);
    ow_write_byte(DS18B20_CMD_CONVERT);
#else
    (void)0;
#endif
}

uint8_t ds18b20_read_temperatures(int16_t temps_c_x100[DS18B20_MAX_SENSORS]) {
    uint8_t valid = 0;
    for (uint8_t i = 0; i < DS18B20_MAX_SENSORS; i++) {
        temps_c_x100[i] = ZCL_TEMPERATURE_INVALID;
#if defined(HAL_TELINK) || defined(HAL_SILABS)
        if ((seen_mask & (1u << i)) == 0 || slots.rom[i][0] != DS18B20_FAMILY) {
            continue;
        }
        int16_t raw = 0;
        if (!read_scratchpad(slots.rom[i], &raw)) {
            continue;
        }
        temps_c_x100[i] = (int16_t)(((int32_t)raw * 100) / 16);
        valid++;
#endif
    }
    return valid;
}

#if defined(HAL_TELINK) || defined(HAL_SILABS)
static uint8_t ds2438_read_humidity(const uint8_t rom[8], uint16_t *rh_percent_x100) {
    uint8_t page[8];
    if (!ds2438_read_page(rom, 0, page)) {
        return 0;
    }
    int16_t temp_raw    = (int16_t)((uint16_t)page[1] | ((uint16_t)page[2] << 8));
    int32_t temp_c_x100 = ((int32_t)temp_raw * 100) / 256;

    uint16_t vad_mv = 0;
    uint16_t vdd_mv = 0;
    if (!ds2438_read_voltage(rom, 1, &vad_mv) ||
        !ds2438_read_voltage(rom, 0, &vdd_mv) || vdd_mv < 1000) {
        return 0;
    }

    /*
     * HIH-4000 / HIH-3610 on a DS2438 VAD pin, ratiometric to VDD:
     * sensor_rh = (VAD/VDD - 0.16) / 0.0062
     * true_rh = sensor_rh / (1.0546 - 0.00216 * T)
     * The integer form below is centi-percent (0.01 %RH).
     */
    int32_t sensor_centi = ((int32_t)vad_mv * 16129) / (int32_t)vdd_mv - 2581;
    int32_t denom        = 10546000 - 216 * temp_c_x100;
    if (denom < 1000000) {
        return 0;
    }
    int32_t rh = (int32_t)(((int64_t)sensor_centi * 10000000) / denom);
    if (rh < 0) {
        rh = 0;
    } else if (rh > 10000) {
        rh = 10000;
    }
    *rh_percent_x100 = (uint16_t)rh;
    return 1;
}
#endif

uint8_t ds18b20_read_humidities(uint16_t rh_percent_x100[DS18B20_MAX_SENSORS]) {
    uint8_t valid = 0;
    for (uint8_t i = 0; i < DS18B20_MAX_SENSORS; i++) {
        rh_percent_x100[i] = ZCL_HUMIDITY_INVALID;
#if defined(HAL_TELINK) || defined(HAL_SILABS)
        if ((seen_mask & (1u << i)) == 0 || slots.rom[i][0] != DS2438_FAMILY) {
            continue;
        }
        if (ds2438_read_humidity(slots.rom[i], &rh_percent_x100[i])) {
            valid++;
        }
#endif
    }
    return valid;
}
