#include "onewire.h"

#include "ds18b20.h"
#include "ds2438.h"
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

typedef struct {
    uint8_t magic;
    uint8_t occupied;
    uint8_t rom[OW_MAX_SENSORS][8];
} ow_nv_t;

static hal_gpio_pin_t bus_pin = HAL_INVALID_PIN;
static ow_nv_t        slots;
static uint8_t        seen_mask;
static uint8_t        new_mask;
/* Reset low time that found each slot. A DS18B20 answers 600 µs. The
 * ATtiny answers a different length and ignores the other one. */
static uint16_t slot_low_us[OW_MAX_SENSORS];
static uint16_t pending_low_us[OW_MAX_SENSORS];
uint16_t        ow_active_low;
static uint16_t ow_learned_us;
uint16_t        ow_use_low_us;
static uint8_t  ow_log_errors = 1;

static ow_reading_t details[OW_MAX_SENSORS];

static void ow_rom_to_text(const uint8_t rom[8], char out[24]) {
    static const char hex[] = "0123456789abcdef";
    uint8_t n = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (i != 0) {
            out[n++] = '-';
        }
        out[n++] = hex[rom[i] >> 4];
        out[n++] = hex[rom[i] & 0x0f];
    }
    out[n] = '\0';
}

static void printf_centi(int32_t value) {
    if (value < 0) {
        printf("-");
        value = -value;
    }
    printf("%d.%02d", (int)(value / 100), (int)(value % 100));
}

uint8_t ow_crc8(const uint8_t *data, uint8_t len) {
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

void ow_delay_us(uint32_t us) {
#if defined(HAL_TELINK)
    sleep_us(us);
#else
    volatile uint32_t count = us * 8;
    while (count--) {}
#endif
}

uint32_t ow_irq_save(void) {
#if defined(HAL_TELINK)
    return irq_disable();
#else
    return 0;
#endif
}

void ow_irq_restore(uint32_t state) {
#if defined(HAL_TELINK)
    irq_restore(state);
#else
    (void)state;
#endif
}

static void ow_drive_low(void) {
#if defined(HAL_TELINK)
    GPIO_PinTypeDef pin = (GPIO_PinTypeDef)bus_pin;
    /*
     * Output enable is a single register write. Input enable on port B is
     * an analog transaction and would swallow a short slot, so it stays on
     * from ow_init.
     */
    gpio_write(pin, 0);
    gpio_set_output_en(pin, 1);
#else
    hal_gpio_init(bus_pin, 0, HAL_GPIO_PULL_NONE);
    hal_gpio_clear(bus_pin);
#endif
}

static void ow_release(void) {
#if defined(HAL_TELINK)
    gpio_set_output_en((GPIO_PinTypeDef)bus_pin, 0);
#else
    hal_gpio_init(bus_pin, 1, HAL_GPIO_PULL_NONE);
#endif
}

static uint8_t ow_sample(void) {
#if defined(HAL_TELINK)
    return gpio_read((GPIO_PinTypeDef)bus_pin) ? 1 : 0;
#else
    return hal_gpio_read(bus_pin);
#endif
}

static uint8_t ow_presence(uint16_t low_us) {
    /*
     * A slow rise is still low at 15 µs and used to count as a device.
     * Presence is a low that starts after the line has been high once.
     * The watch runs to 1.5 ms. The next slot waits until the pulse ends.
     */
    uint32_t state = ow_irq_save();
    ow_drive_low();
    ow_delay_us(low_us);
    ow_release();
    uint8_t present   = 0;
    uint8_t seen_high = 0;
#if defined(HAL_TELINK)
    unsigned int start = (unsigned int)clock_time();
    do {
        if (ow_sample()) {
            if (present) {
                break;
            }
            seen_high = 1;
        } else if (seen_high) {
            present = 1;
        }
    } while (!clock_time_exceed(start, 1500));
    if (present && ow_sample() == 0) {
        unsigned int rise = (unsigned int)clock_time();
        while (ow_sample() == 0 && !clock_time_exceed(rise, 500)) {
        }
    }
#else
    uint16_t elapsed = 0;
    while (elapsed < 1500) {
        if (ow_sample()) {
            if (present) {
                break;
            }
            seen_high = 1;
        } else if (seen_high) {
            present = 1;
        }
        ow_delay_us(10);
        elapsed = (uint16_t)(elapsed + 10);
    }
#endif
    ow_irq_restore(state);
    ow_delay_us(30);
    return present;
}

uint8_t ow_reset(void) {
    /*
     * 600 µs is the low this DS18B20 answers. The ATtiny sends no presence
     * for that pulse: a software slave drops a reset outside its own
     * window, and the ESP32 master it does answer holds the line for a
     * different time. Other lengths are sent only while the line stays
     * high, then the one that worked is reused.
     */
    static const uint16_t lows[] = {600, 480, 360, 900, 1400, 2200, 4000, 7000};
    if (ow_use_low_us != 0) {
        ow_active_low = ow_use_low_us;
        return ow_presence(ow_use_low_us);
    }
    if (ow_learned_us != 0 && ow_presence(ow_learned_us)) {
        ow_active_low = ow_learned_us;
        return 1;
    }
    for (uint8_t pass = 0; pass < 2; pass++) {
        for (uint8_t i = 0; i < (uint8_t)(sizeof(lows) / sizeof(lows[0])); i++) {
            if (lows[i] == ow_learned_us) {
                continue;
            }
            if (ow_presence(lows[i])) {
                ow_active_low = lows[i];
                return 1;
            }
        }
    }
    ow_active_low = 0;
    return 0;
}

static void ow_write_bit(uint8_t bit) {
    /* Release and let the board pull-up rise. Driving the pin high made
     * the DS18B20 stop answering. A write-1 is low for 6 µs, which that
     * sensor accepts, then high for the rest of the slot. */
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
    /* Sample at 15 µs before any other work. A DS18B20 may release a 0
     * at that instant, and the search then sees two high bits and stops.
     * The spin after that catches an ATtiny that pulls later. The slot
     * is still 70 µs. */
    uint32_t state = ow_irq_save();
    ow_drive_low();
    ow_delay_us(6);
    ow_release();
    ow_delay_us(9);
    uint8_t bit = ow_sample() ? 1 : 0;
#if defined(HAL_TELINK)
    unsigned int start = (unsigned int)clock_time();
    do {
        if (ow_sample() == 0) {
            bit = 0;
        }
    } while (!clock_time_exceed(start, 45));
#else
    ow_delay_us(45);
#endif
    ow_irq_restore(state);
    ow_delay_us(10);
    return bit;
}

void ow_write_byte(uint8_t value) {
    for (uint8_t bit = 0; bit < 8; bit++) {
        ow_write_bit(value & 0x01);
        value >>= 1;
    }
}

uint8_t ow_read_byte(void) {
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
 * Returns 1 on a good ROM, 0 on CRC failure or a dead bus, 2 when bit 1
 * had no slave so the caller can try another reset length.
 */
static uint8_t ow_search_command(uint8_t rom[8], uint8_t *last_discrepancy,
                                 uint8_t *last_device) {
    ow_write_byte(OW_CMD_SEARCH);

    uint8_t last_zero   = 0;
    uint8_t rom_byte    = 0;
    uint8_t rom_mask    = 1;
    uint8_t bit_number  = 1;

    for (uint8_t i = 0; i < 64; i++) {
        uint8_t bit     = ow_read_bit();
        uint8_t complement = ow_read_bit();
        uint8_t direction;

        if (bit && complement) {
            if (bit_number == 1) {
                return 2;
            }
            if (ow_log_errors) {
                printf("1-Wire no search response at bit %d\r\n", bit_number);
            }
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
    return ow_crc8(rom, 7) == rom[7] ? 1 : 0;
}

static uint8_t ow_search_next(uint8_t rom[8], uint8_t *last_discrepancy,
                              uint8_t *last_device) {
    static const uint16_t lows[] = {600, 480, 360, 900, 1400, 2200, 4000, 7000};
    if (*last_device) {
        return 0;
    }

    /*
     * A low after one pulse is not enough. The ATtiny log was presence
     * on the 600 µs pulse, then both search bits high, and the scan
     * stopped. Bit 1 has to be pulled before that length is kept.
     */
    if (ow_use_low_us != 0) {
        if (!ow_presence(ow_use_low_us)) {
            if (ow_log_errors) {
                printf("1-Wire no presence (reset 360-7000 us)\r\n");
            }
            *last_discrepancy = 0;
            *last_device      = 1;
            return 0;
        }
        ow_active_low = ow_use_low_us;
        uint8_t forced = ow_search_command(rom, last_discrepancy, last_device);
        if (forced == 2) {
            if (ow_log_errors) {
                printf("1-Wire no search response at bit 1\r\n");
            }
            *last_discrepancy = 0;
            *last_device      = 1;
            return 0;
        }
        if (forced == 1) {
            ow_learned_us = ow_use_low_us;
        }
        return forced;
    }

    uint16_t try_us[8];
    uint8_t ntry = 0;
    if (ow_learned_us != 0) {
        try_us[ntry++] = ow_learned_us;
    }
    for (uint8_t i = 0; i < (uint8_t)(sizeof(lows) / sizeof(lows[0])); i++) {
        if (lows[i] != ow_learned_us) {
            try_us[ntry++] = lows[i];
        }
    }

    uint8_t saw_presence = 0;
    for (uint8_t pass = 0; pass < 2; pass++) {
        for (uint8_t i = 0; i < ntry; i++) {
            if (!ow_presence(try_us[i])) {
                continue;
            }
            saw_presence  = 1;
            ow_active_low = try_us[i];
            uint8_t st = ow_search_command(rom, last_discrepancy, last_device);
            if (st == 2) {
                continue;
            }
            ow_learned_us = try_us[i];
            return st;
        }
    }

    if (ow_log_errors) {
        if (!saw_presence) {
            printf("1-Wire no presence (reset 360-7000 us)\r\n");
        } else {
            printf("1-Wire no search response at bit 1\r\n");
        }
    }
    *last_discrepancy = 0;
    *last_device      = 1;
    return 0;
}

#endif

int ow_rom_slot(const uint8_t rom[8]) {
    for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
        if ((slots.occupied & (1u << i)) &&
            memcmp(slots.rom[i], rom, 8) == 0) {
            return i;
        }
    }
    return -1;
}

uint16_t ow_slot_reset_us(int slot) {
    if (slot < 0 || slot >= OW_MAX_SENSORS) {
        return 0;
    }
    return slot_low_us[slot];
}

void ow_set_slot_reset_us(int slot, uint16_t us) {
    if (slot >= 0 && slot < OW_MAX_SENSORS) {
        slot_low_us[slot] = us;
    }
}

static int first_free_slot(void) {
    for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
        if ((slots.occupied & (1u << i)) == 0) {
            return i;
        }
    }
    return -1;
}

static void save_slots(void) {
    slots.magic = OW_NV_MAGIC;
    hal_nvm_status_t status =
        hal_nvm_write(NV_ITEM_DS18B20_SLOTS, sizeof(slots), (uint8_t *)&slots);
    if (status != HAL_NVM_SUCCESS) {
        printf("1-Wire slot save failed: %d\r\n", status);
    }
}

static void load_slots(void) {
    memset(&slots, 0, sizeof(slots));
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    ow_nv_t stored;
    if (hal_nvm_read(NV_ITEM_DS18B20_SLOTS, sizeof(stored),
                     (uint8_t *)&stored) == HAL_NVM_SUCCESS &&
        stored.magic == OW_NV_MAGIC) {
        slots = stored;
    }
#endif
}

/* ROMs found on this search that do not yet have a slot. Assigned after the
 * search, so a reservation is reused only when its sensor never answered. */
static uint8_t pending_roms[OW_MAX_SENSORS][8];
static uint8_t pending_count;

static int first_absent_slot(void) {
    for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
        if ((slots.occupied & (1u << i)) && (seen_mask & (1u << i)) == 0) {
            return i;
        }
    }
    return -1;
}

static void note_found(const uint8_t rom[8]) {
    uint16_t low_us = ow_active_low != 0 ? ow_active_low : 600;
    int slot = ow_rom_slot(rom);
    if (slot >= 0) {
        seen_mask |= (uint8_t)(1u << slot);
        if (slot_low_us[slot] == 0) {
            slot_low_us[slot] = low_us;
        }
        return;
    }
    for (uint8_t i = 0; i < pending_count; i++) {
        if (memcmp(pending_roms[i], rom, 8) == 0) {
            return;
        }
    }
    if (pending_count >= OW_MAX_SENSORS) {
        char id[24];
        ow_rom_to_text(rom, id);
        printf("1-Wire bus full, no slot for id %s\r\n", id);
        return;
    }
    memcpy(pending_roms[pending_count], rom, 8);
    pending_low_us[pending_count] = low_us;
    pending_count++;
    if (low_us != 600) {
        char id[24];
        ow_rom_to_text(rom, id);
        printf("1-Wire id %s answered %d us reset\r\n", id, (int)low_us);
    }
}

static void assign_pending(void) {
    uint8_t changed = 0;
    for (uint8_t n = 0; n < pending_count; n++) {
        int slot = first_free_slot();
        uint8_t replaced = 0;
        char old_id[24];
        if (slot < 0) {
            slot = first_absent_slot();
            replaced = slot >= 0;
        }
        if (slot < 0) {
            char id[24];
            ow_rom_to_text(pending_roms[n], id);
            printf("1-Wire bus full, no slot for id %s\r\n", id);
            continue;
        }
        if (replaced) {
            ow_rom_to_text(slots.rom[slot], old_id);
        }
        memcpy(slots.rom[slot], pending_roms[n], 8);
        slot_low_us[slot] = pending_low_us[n];
        slots.occupied |= (uint8_t)(1u << slot);
        new_mask |= (uint8_t)(1u << slot);
        seen_mask |= (uint8_t)(1u << slot);
        changed = 1;
        if (replaced) {
            char id[24];
            ow_rom_to_text(pending_roms[n], id);
            printf("1-Wire slot %d replaced %s with %s\r\n", slot, old_id, id);
        }
    }
    pending_count = 0;
    if (changed) {
        save_slots();
    }
}

#if defined(HAL_TELINK) || defined(HAL_SILABS)

uint8_t ow_reset_for(const uint8_t rom[8]) {
    int slot = ow_rom_slot(rom);
    uint16_t prev = ow_use_low_us;
    if (slot >= 0 && slot_low_us[slot] != 0) {
        ow_use_low_us = slot_low_us[slot];
        if (ow_reset()) {
            ow_use_low_us = prev;
            return 1;
        }
    }
    /* The length saved at search did not get a presence. Try the others. */
    ow_use_low_us = 0;
    uint8_t ok = ow_reset();
    ow_use_low_us = prev;
    if (ok && slot >= 0 && ow_active_low != 0) {
        slot_low_us[slot] = ow_active_low;
    }
    return ok;
}

uint8_t ow_scratch_empty(const uint8_t scratch[9]) {
    uint8_t zeros = 1;
    uint8_t ones  = 1;
    for (uint8_t i = 0; i < 9; i++) {
        if (scratch[i] != 0x00) {
            zeros = 0;
        }
        if (scratch[i] != 0xFF) {
            ones = 0;
        }
    }
    /* CRC of eight 0x00 bytes is 0x00, so a blank page looks valid. */
    return zeros || ones;
}

uint8_t ow_scratch_crc_ok(const uint8_t scratch[9]) {
    return (uint8_t)(!ow_scratch_empty(scratch) &&
                     ow_crc8(scratch, 8) == scratch[8]);
}

uint8_t ow_match_cmd(const uint8_t rom[8], uint8_t cmd) {
    uint32_t state = ow_irq_save();
    uint8_t ok     = ow_reset_for(rom);
    if (ok) {
        ow_write_byte(OW_CMD_MATCH);
        for (uint8_t i = 0; i < 8; i++) {
            ow_write_byte(rom[i]);
        }
        ow_write_byte(cmd);
    }
    ow_irq_restore(state);
    return ok;
}

uint8_t ow_match(const uint8_t rom[8]) {
    if (!ow_reset_for(rom)) {
        return 0;
    }
    ow_write_byte(OW_CMD_MATCH);
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_byte(rom[i]);
    }
    return 1;
}

#endif

void ow_init(hal_gpio_pin_t pin) {
    bus_pin = pin;
    load_slots();
#if defined(HAL_TELINK)
    gpio_set_func((GPIO_PinTypeDef)pin, AS_GPIO);
#endif
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    hal_gpio_init(pin, 1, HAL_GPIO_PULL_NONE);
#if defined(HAL_TELINK)
    gpio_write((GPIO_PinTypeDef)pin, 0);
    gpio_set_data_strength((GPIO_PinTypeDef)pin, 1);
#endif
#else
    (void)pin;
#endif
}

uint8_t ow_is_configured(void) {
    return bus_pin != HAL_INVALID_PIN;
}

/* Stuck-bus guard. The search itself stops when the bus reports the last
 * device, not at a smaller sensor count. Eight stored sensors fit in the table. */
#define OW_SEARCH_STEP_LIMIT    32

static uint8_t scan_rom[8];
static uint8_t scan_last_discrepancy;
static uint8_t scan_last_device;
static uint8_t scan_active;
static uint8_t scan_steps;
static uint8_t read_cursor;
static uint8_t read_active;

void ow_search_begin(void) {
    seen_mask             = 0;
    new_mask              = 0;
    scan_last_discrepancy = 0;
    scan_last_device      = 0;
    scan_steps            = 0;
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    pending_count         = 0;
#endif
    scan_active           = ow_is_configured();
    memset(scan_rom, 0, sizeof(scan_rom));
    memset(slot_low_us, 0, sizeof(slot_low_us));
    ow_learned_us = 0;
}

uint8_t ow_search_step(void) {
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    if (!scan_active) {
        return 0;
    }
    if (scan_steps >= OW_SEARCH_STEP_LIMIT) {
        printf("1-Wire search stopped after %d steps\r\n", scan_steps);
        assign_pending();
        scan_active = 0;
        return 0;
    }
    scan_steps++;

    if (!ow_search_next(scan_rom, &scan_last_discrepancy, &scan_last_device)) {
        if (scan_last_device) {
            assign_pending();
            scan_active = 0;
            return 0;
        }
        /* CRC failed. The boot search retries this immediately. */
        return 1;
    }
    if (scan_rom[0] == DS18B20_FAMILY || scan_rom[0] == DS2438_FAMILY) {
        note_found(scan_rom);
    } else {
        char id[24];
        ow_rom_to_text(scan_rom, id);
        printf("1-Wire unknown id %s family 0x%02x\r\n", id, scan_rom[0]);
    }
    if (scan_last_device) {
        assign_pending();
        scan_active = 0;
        return 0;
    }
    return 1;
#else
    scan_active = 0;
    return 0;
#endif
}

void ow_search_alternate_resets(void) {
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    /*
     * The first scan keeps whichever reset pulled search bit 1. A second
     * slave can ignore that pulse, so its ROM never makes a discrepancy
     * and the scan stops without it. Search every other length and keep
     * the devices already found. 600 µs is included when it was not the
     * one that won the first scan.
     */
    if (!ow_is_configured() || seen_mask == 0 || ow_learned_us == 0) {
        return;
    }
    static const uint16_t alts[] = {600, 480, 360, 900, 1400, 2200, 4000, 7000};
    uint16_t kept = ow_learned_us;
    printf("1-Wire trying other reset lengths, kept %d us\r\n", (int)kept);
    ow_log_errors = 0;
    for (uint8_t i = 0; i < (uint8_t)(sizeof(alts) / sizeof(alts[0])); i++) {
        if (alts[i] == kept) {
            continue;
        }
        ow_use_low_us         = alts[i];
        scan_last_discrepancy = 0;
        scan_last_device      = 0;
        scan_steps            = 0;
        scan_active           = 1;
        memset(scan_rom, 0, sizeof(scan_rom));
        while (ow_search_step()) {
        }
    }
    ow_use_low_us = 0;
    ow_learned_us = kept;
    ow_log_errors = 1;
#endif
}

uint8_t ow_seen_mask(void) {
    return seen_mask;
}

uint8_t ow_slot_family(uint8_t slot) {
    if (slot >= OW_MAX_SENSORS || (seen_mask & (1u << slot)) == 0) {
        return 0;
    }
    return slots.rom[slot][0];
}

uint8_t ow_slot_id(uint8_t slot, char out[24]) {
    if (out == NULL || slot >= OW_MAX_SENSORS ||
        (seen_mask & (1u << slot)) == 0) {
        return 0;
    }
    ow_rom_to_text(slots.rom[slot], out);
    return 1;
}

uint8_t ow_sensors_seen(void) {
    return seen_mask != 0;
}

uint8_t ow_convert_start(void) {
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    /*
     * Match each sensor. Skip ROM is ignored by a slave that is emulating
     * more than one device, so the convert never starts and the scratchpad
     * stays empty. A real DS18B20 accepts Match + Convert as well.
     */
    uint8_t started = 0;
    for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
        if ((seen_mask & (1u << i)) == 0) {
            continue;
        }
        if (slots.rom[i][0] != DS18B20_FAMILY &&
            slots.rom[i][0] != DS2438_FAMILY) {
            continue;
        }
        /* Convert V on the humidity ROM, before this read, made the
         * temperature scratchpad come back as 0xFF. Convert T only. */
        if (ow_match_cmd(slots.rom[i], OW_CMD_CONVERT)) {
            started = 1;
        }
    }
    return started;
#else
    return 0;
#endif
}

void ow_read_begin(int16_t temps_c_x100[OW_MAX_SENSORS],
                   uint16_t rh_percent_x100[OW_MAX_SENSORS]) {
    for (uint8_t i = 0; i < OW_MAX_SENSORS; i++) {
        temps_c_x100[i]    = ZCL_TEMPERATURE_INVALID;
        rh_percent_x100[i] = ZCL_HUMIDITY_INVALID;
    }
    read_cursor = 0;
    read_active = 1;
}

uint8_t ow_read_step(int16_t temps_c_x100[OW_MAX_SENSORS],
                     uint16_t rh_percent_x100[OW_MAX_SENSORS]) {
    if (!read_active) {
        return 0;
    }
    while (read_cursor < OW_MAX_SENSORS &&
           (seen_mask & (1u << read_cursor)) == 0) {
        read_cursor++;
    }
    if (read_cursor >= OW_MAX_SENSORS) {
        read_active = 0;
        return 0;
    }

    uint8_t slot = read_cursor++;
    memset(&details[slot], 0, sizeof(details[slot]));
#if defined(HAL_TELINK) || defined(HAL_SILABS)
    if (slots.rom[slot][0] == DS18B20_FAMILY) {
        ds18b20_read(slots.rom[slot], &details[slot], &temps_c_x100[slot]);
    } else if (slots.rom[slot][0] == DS2438_FAMILY) {
        ds2438_read(slots.rom[slot], &details[slot], &rh_percent_x100[slot]);
        if (details[slot].have_temp) {
            temps_c_x100[slot] = details[slot].temp_c_x100;
        }
    }
#else
    (void)slot;
    (void)temps_c_x100;
    (void)rh_percent_x100;
#endif

    for (uint8_t i = read_cursor; i < OW_MAX_SENSORS; i++) {
        if (seen_mask & (1u << i)) {
            return 1;
        }
    }
    read_active = 0;
    return 0;
}

void ow_log_slot(uint8_t slot, uint8_t endpoint) {
    if (slot >= OW_MAX_SENSORS || (seen_mask & (1u << slot)) == 0) {
        return;
    }
    char id[24];
    ow_rom_to_text(slots.rom[slot], id);
    const char *kind = "sensor";
    if (slots.rom[slot][0] == DS18B20_FAMILY) {
        kind = "DS18B20";
    } else if (slots.rom[slot][0] == DS2438_FAMILY) {
        kind = "DS2438";
    }
    printf("1-Wire %s slot %d %s id %s ep %d", kind, slot,
           (new_mask & (1u << slot)) ? "new" : "saved", id, endpoint);
    if (details[slot].have_temp) {
        printf(" temp ");
        printf_centi(details[slot].temp_c_x100);
        printf("C");
    }
    if (details[slot].have_alarms) {
        printf(" res %d-bit th %dC tl %dC", details[slot].resolution,
               details[slot].alarm_high, details[slot].alarm_low);
    }
    if (slots.rom[slot][0] == DS18B20_FAMILY && details[slot].have_temp) {
        printf(" scratch");
        for (uint8_t i = 0; i < 9; i++) {
            printf(" %02x", details[slot].raw[i]);
        }
    }
    if (details[slot].have_rh) {
        printf(" rh ");
        printf_centi(details[slot].rh_percent_x100);
        printf("%%RH");
    }
    if (slots.rom[slot][0] == DS2438_FAMILY && details[slot].raw_len != 0) {
        printf(" bytes");
        for (uint8_t i = 0; i < details[slot].raw_len; i++) {
            printf(" %02x", details[slot].raw[i]);
        }
    }
    if (details[slot].have_supply) {
        printf(" vad %dmV vdd %dmV", details[slot].vad_mv, details[slot].vdd_mv);
    }
    if (!details[slot].have_temp && !details[slot].have_rh) {
        if (details[slot].raw_err == 1) {
            printf(" no reading, no presence");
        } else if (details[slot].raw_err == 2 &&
                   slots.rom[slot][0] != DS2438_FAMILY) {
            printf(" no reading, scratch");
            for (uint8_t i = 0; i < 9; i++) {
                printf(" %02x", details[slot].raw[i]);
            }
        } else if (details[slot].raw_err == 2) {
            printf(" no reading");
        } else {
            printf(" no reading");
        }
    }
    printf("\r\n");
}