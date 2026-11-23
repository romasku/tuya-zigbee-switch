#ifndef _ONEWIRE_BUS_H_
#define _ONEWIRE_BUS_H_

#include "onewire.h"
#include <stdint.h>

#define OW_CMD_SEARCH        0xF0
#define OW_CMD_MATCH         0x55
#define OW_CMD_CONVERT       0x44
#define OW_CMD_READ_SCRATCH  0xBE

#define OW_NV_MAGIC          0xB6

typedef struct {
    int16_t  temp_c_x100;
    uint16_t rh_percent_x100;
    uint16_t vad_mv;
    uint16_t vdd_mv;
    int8_t   alarm_high;
    int8_t   alarm_low;
    uint8_t  resolution;
    uint8_t  have_temp;
    uint8_t  have_rh;
    uint8_t  have_supply;
    uint8_t  have_alarms;
    uint8_t  raw_err;
    uint8_t  raw_len;
    uint8_t  raw[96];
} ow_reading_t;

/* Reset length forced by the caller of ow_reset. 0 selects the learned one. */
extern uint16_t ow_use_low_us;
extern uint16_t ow_active_low;

void ow_delay_us(uint32_t us);
uint32_t ow_irq_save(void);
void ow_irq_restore(uint32_t state);
uint8_t ow_reset(void);
uint8_t ow_reset_for(const uint8_t rom[8]);
void ow_write_byte(uint8_t value);
uint8_t ow_read_byte(void);
uint8_t ow_match(const uint8_t rom[8]);
uint8_t ow_match_cmd(const uint8_t rom[8], uint8_t cmd);
uint8_t ow_crc8(const uint8_t *data, uint8_t len);
uint8_t ow_scratch_empty(const uint8_t scratch[9]);
uint8_t ow_scratch_crc_ok(const uint8_t scratch[9]);
int ow_rom_slot(const uint8_t rom[8]);
uint16_t ow_slot_reset_us(int slot);
void ow_set_slot_reset_us(int slot, uint16_t us);

#endif
