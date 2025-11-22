#pragma pack(push, 1)
#include "chip_8258/flash.h"
#include "tl_common.h"
#pragma pack(pop)

// Definitions of private function are from SDK's flash.c:

_attribute_ram_code_sec_noinline_ void
flash_mspi_read_ram(unsigned char cmd, unsigned long addr,
                    unsigned char addr_en, unsigned char dummy_cnt,
                    unsigned char *data, unsigned long data_len);

_attribute_ram_code_sec_noinline_ unsigned char
flash_mspi_write_ram(unsigned char cmd, unsigned long addr,
                     unsigned char addr_en, unsigned char *data,
                     unsigned long data_len);

// Ram code reimplementations of flash functions:

void _attribute_ram_code_sec_ ram_code_flash_read_page(unsigned long addr,
                                                       unsigned long len,
                                                       unsigned char *buf) {
  flash_mspi_read_ram(FLASH_READ_CMD, addr, 1, 0, buf, len);
}

void _attribute_ram_code_sec_ ram_code_flash_write_page(unsigned long addr,
                                                        unsigned long len,
                                                        unsigned char *buf) {
  unsigned int ns = PAGE_SIZE - (addr & (PAGE_SIZE - 1));
  int nw = 0;

  do {
    nw = len > ns ? ns : len;
    if (flash_mspi_write_ram(FLASH_WRITE_CMD, addr, 1, buf, nw) == 0) {
      break;
    }
    ns = PAGE_SIZE;
    addr += nw;
    buf += nw;
    len -= nw;
  } while (len > 0);
}

void _attribute_ram_code_sec_ ram_code_flash_erase_sector(unsigned long addr) {
  flash_mspi_write_ram(FLASH_SECT_ERASE_CMD, addr, 1, NULL, 0);
}

void _attribute_ram_code_sec_
ram_code_flash_write_status(flash_status_typedef_e type, unsigned short data) {
  unsigned char buf[2];

  buf[0] = data;
  buf[1] = data >> 8;
  if (type == FLASH_TYPE_8BIT_STATUS) {
    flash_mspi_write_ram(FLASH_WRITE_STATUS_CMD_LOWBYTE, 0, 0, buf, 1);
  } else if (type == FLASH_TYPE_16BIT_STATUS_ONE_CMD) {
    flash_mspi_write_ram(FLASH_WRITE_STATUS_CMD_LOWBYTE, 0, 0, buf, 2);
  }
}