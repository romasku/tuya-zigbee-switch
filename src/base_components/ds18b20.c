#include "ds18b20.h"

#include <string.h>

#if defined(HAL_TELINK) || defined(HAL_SILABS)

/* 0: no presence. 1: bytes received, CRC mismatch. 2: CRC ok. */
static uint8_t ds18b20_read_scratchpad_at(const uint8_t rom[8], uint8_t scratch[9],
                                          uint16_t low_us) {
    memset(scratch, 0xFF, 9);
    /*
     * Hold interrupts across this one command. A radio IRQ in the gap
     * between bits makes this slave drop the Match, and the scratchpad
     * then reads as 0xFF. Search ran before the radio and did not see that.
     */
    uint32_t state  = ow_irq_save();
    uint16_t prev   = ow_use_low_us;
    ow_use_low_us   = low_us;
    uint8_t present = ow_reset();
    ow_use_low_us   = prev;
    if (present) {
        ow_active_low = low_us;
        ow_write_byte(OW_CMD_MATCH);
        for (uint8_t i = 0; i < 8; i++) {
            ow_write_byte(rom[i]);
        }
        ow_write_byte(OW_CMD_READ_SCRATCH);
        for (uint8_t i = 0; i < 9; i++) {
            scratch[i] = ow_read_byte();
        }
    }
    ow_irq_restore(state);
    return present;
}

/* 0: no presence. 1: bytes received, CRC mismatch. 2: CRC ok.
 * All 0xFF is not a scratchpad: the slave did not drive the bus. A reset
 * length that only produces that must not block the lengths that do. */
static uint8_t ds18b20_read_scratchpad_lengths(const uint8_t rom[8],
                                               uint8_t scratch[9]) {
    static const uint16_t lows[] = {600, 480, 360, 900, 1400, 2200, 4000, 7000};
    int slot = ow_rom_slot(rom);
    uint16_t try_us[8];
    uint8_t ntry    = 0;
    uint8_t present = 0;
    uint16_t first  = (slot >= 0) ? ow_slot_reset_us(slot) : 0;
    memset(scratch, 0xFF, 9);
    if (first != 0) {
        try_us[ntry++] = first;
    }
    for (uint8_t i = 0; i < (uint8_t)(sizeof(lows) / sizeof(lows[0])); i++) {
        if (lows[i] != first && ntry < 8) {
            try_us[ntry++] = lows[i];
        }
    }
    for (uint8_t i = 0; i < ntry; i++) {
        uint8_t got[9];
        if (!ds18b20_read_scratchpad_at(rom, got, try_us[i])) {
            continue;
        }
        present = 1;
        memcpy(scratch, got, 9);
        if (ow_scratch_crc_ok(got) || !ow_scratch_empty(got)) {
            if (slot >= 0) {
                ow_set_slot_reset_us(slot, try_us[i]);
            }
            return ow_scratch_crc_ok(got) ? 2 : 1;
        }
    }
    return present ? 1 : 0;
}

static uint8_t ds18b20_read_scratchpad(const uint8_t rom[8], uint8_t scratch[9]) {
    uint8_t st = ds18b20_read_scratchpad_lengths(rom, scratch);
    if (st == 2 || (st == 1 && !ow_scratch_empty(scratch))) {
        return st;
    }
    ow_delay_us(20000);
    uint8_t again = ds18b20_read_scratchpad_lengths(rom, scratch);
    return again != 0 ? again : st;
}

#endif

void ds18b20_read(const uint8_t rom[8], ow_reading_t *reading,
                  int16_t *temp_c_x100) {
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    uint8_t scratch[9];
    uint8_t st = ds18b20_read_scratchpad(rom, scratch);
    reading->raw_err = st == 2 ? 0 : (st == 0 ? 1 : 2);
    memcpy(reading->raw, scratch, 9);
    if (st == 2) {
        int16_t raw = (int16_t)((uint16_t)scratch[0] | ((uint16_t)scratch[1] << 8));
        *temp_c_x100           = (int16_t)(((int32_t)raw * 100) / 16);
        reading->temp_c_x100   = *temp_c_x100;
        reading->have_temp     = 1;
        reading->alarm_high    = (int8_t)scratch[2];
        reading->alarm_low     = (int8_t)scratch[3];
        reading->resolution    = (uint8_t)(((scratch[4] >> 5) & 0x03) + 9);
        reading->have_alarms   = 1;
    }
#else
    (void)rom;
    (void)reading;
    (void)temp_c_x100;
#endif
}