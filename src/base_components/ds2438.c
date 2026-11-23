#include "ds2438.h"

#include <string.h>

#define DS2438_CMD_WRITE      0x4E
#define DS2438_CMD_COPY       0x48
#define DS2438_CMD_RECALL     0xB8
#define DS2438_CMD_CONVERTV   0xB4
#define DS2438_PAGE0          0
#define DS2438_STATUS_AD      0x08
/* Same pauses as DS2438::startConversion(). Copy holds the line low. */
#define DS2438_TEMP_WAIT_US   10000
#define DS2438_VOLT_WAIT_US   8000
#define DS2438_COPY_WAIT_US   10000

#if defined(HAL_TELINK) || defined(HAL_SILABS)

static uint8_t ds2438_blank(const uint8_t page[9]) {
    uint8_t zeros = 1;
    uint8_t ones  = 1;
    for (uint8_t i = 0; i < 8; i++) {
        if (page[i] != 0x00) {
            zeros = 0;
        }
        if (page[i] != 0xFF) {
            ones = 0;
        }
    }
    return zeros || ones;
}

/* 0: no presence. The buffer is filled either way. */
static uint8_t ds2438_read_page(const uint8_t rom[8], uint8_t *data) {
    memset(data, 0xFF, 9);
    uint32_t state  = ow_irq_save();
    uint8_t present = ow_match(rom);
    if (present) {
        ow_write_byte(DS2438_CMD_RECALL);
        ow_write_byte(DS2438_PAGE0);
    }
    ow_irq_restore(state);
    if (!present) {
        return 0;
    }
    state   = ow_irq_save();
    present = ow_match(rom);
    if (present) {
        ow_write_byte(OW_CMD_READ_SCRATCH);
        ow_write_byte(DS2438_PAGE0);
        for (uint8_t i = 0; i < 9; i++) {
            data[i] = ow_read_byte();
        }
    }
    ow_irq_restore(state);
    return present;
}

/* Read-modify of the status byte, then Copy. Other bytes stay as read. */
static uint8_t ds2438_select_channel(const uint8_t rom[8], uint8_t vdd) {
    uint8_t page[9];
    /* All-ones still has a valid CRC (0xC9). Do not write that back. */
    if (!ds2438_read_page(rom, page) || ow_crc8(page, 8) != page[8]) {
        return 0;
    }
    uint8_t ones = 1;
    for (uint8_t i = 0; i < 8; i++) {
        if (page[i] != 0xFF) {
            ones = 0;
        }
    }
    if (ones) {
        return 0;
    }
    if (vdd) {
        page[0] = (uint8_t)(page[0] | DS2438_STATUS_AD);
    } else {
        page[0] = (uint8_t)(page[0] & (uint8_t)~DS2438_STATUS_AD);
    }
    uint32_t state = ow_irq_save();
    uint8_t present = ow_match(rom);
    if (present) {
        ow_write_byte(DS2438_CMD_WRITE);
        ow_write_byte(DS2438_PAGE0);
        for (uint8_t i = 0; i < 8; i++) {
            ow_write_byte(page[i]);
        }
    }
    ow_irq_restore(state);
    if (!present) {
        return 0;
    }
    state   = ow_irq_save();
    present = ow_match(rom);
    if (present) {
        ow_write_byte(DS2438_CMD_COPY);
        ow_write_byte(DS2438_PAGE0);
    }
    ow_irq_restore(state);
    ow_delay_us(DS2438_COPY_WAIT_US);
    return present;
}

static uint8_t ds2438_start_conversion(const uint8_t rom[8], uint8_t vdd,
                                       uint8_t do_temp) {
    if (!ds2438_select_channel(rom, vdd)) {
        return 0;
    }
    if (do_temp) {
        if (!ow_match_cmd(rom, OW_CMD_CONVERT)) {
            return 0;
        }
        ow_delay_us(DS2438_TEMP_WAIT_US);
    }
    if (!ow_match_cmd(rom, DS2438_CMD_CONVERTV)) {
        return 0;
    }
    ow_delay_us(DS2438_VOLT_WAIT_US);
    return 1;
}

/* 10 mV counts from page-0 bytes 3 and 4. 0 and all-ones are empty. */
static uint8_t ds2438_voltage(const uint8_t page[9], uint16_t *counts) {
    if (ds2438_blank(page) || ow_crc8(page, 8) != page[8]) {
        return 0;
    }
    uint16_t raw = (uint16_t)(page[3] | ((page[4] & 0x03) << 8));
    if (raw == 0 || raw == 0x03FF) {
        return 0;
    }
    *counts = raw;
    return 1;
}

/*
 * HobbyBoards / DS2438Humidity.ino:
 *   sensor = (Vad / Vdd - 0.16) / 0.0062
 *   RH     = sensor / (1.0546 - 0.00216 * T)
 * Result is ZCL centi-percent.
 */
static uint16_t ds2438_humidity(uint16_t vad, uint16_t vdd, int16_t temp_c_x100) {
    int32_t sensor = ((int32_t)vad * 10000 / (int32_t)vdd - 1600) * 100 / 62;
    int32_t denom  = 10546 - (216 * (int32_t)temp_c_x100) / 1000;
    if (denom < 1000) {
        denom = 1000;
    }
    int32_t pct = sensor * 10000 / denom;
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 10000) {
        pct = 10000;
    }
    return (uint16_t)pct;
}

#endif

void ds2438_read(const uint8_t rom[8], ow_reading_t *reading,
                 uint16_t *rh_percent_x100) {
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    uint8_t page_a[9];
    uint8_t page_b[9];
    memset(page_a, 0xFF, sizeof(page_a));
    memset(page_b, 0xFF, sizeof(page_b));
    /*
     * Channel A is VAD, channel B is VDD. Temperature was already read
     * from the DS18B20 ROM. Convert V here is only to this ROM.
     */
    if (ds2438_select_channel(rom, 0)) {
        ds2438_start_conversion(rom, 0, 1);
    }
    uint8_t present = ds2438_read_page(rom, page_a);
    if (ds2438_select_channel(rom, 1)) {
        ds2438_start_conversion(rom, 1, 0);
        ds2438_read_page(rom, page_b);
    }
    memcpy(reading->raw, page_a, 9);
    memcpy(reading->raw + 9, page_b, 9);
    reading->raw_len = 18;

    uint16_t vad = 0;
    uint16_t vdd = 0;
    if (!present || !ds2438_voltage(page_a, &vad) || !ds2438_voltage(page_b, &vdd)) {
        reading->raw_err = present ? 2 : 1;
        return;
    }
    int16_t temp_c_x100 = 0;
    int16_t raw_t       = (int16_t)(page_a[1] | (page_a[2] << 8));
    if (raw_t != 0) {
        temp_c_x100            = (int16_t)((int32_t)raw_t * 100 / 256);
        reading->temp_c_x100   = temp_c_x100;
        reading->have_temp     = 1;
    }
    reading->vad_mv      = (uint16_t)(vad * 10);
    reading->vdd_mv      = (uint16_t)(vdd * 10);
    reading->have_supply = 1;
    reading->raw_err     = 0;
    uint16_t pct         = ds2438_humidity(vad, vdd, temp_c_x100);
    *rh_percent_x100     = pct;
    reading->rh_percent_x100 = pct;
    reading->have_rh         = 1;
#else
    (void)rom;
    (void)reading;
    (void)rh_percent_x100;
#endif
}