/*** This file ensures that firmware is placed in the correct location in the
 * flash so that OTA updates work correctly.
 *
 * More specifically, it enforces "big ota" no bootloader mode, where the image
 * is image can be either at the start of flash (0x00000) or at the OTA image
 * location (0x40000), and this slots are used alternately for OTA updates.
 *
 * In no-bootloader mode, MCU can boot from either 0x00000 or 0x20000 or
 * 0x40000, so we should ensure that 0x00000 or 0x40000 is used.
 *
 * In bootloader mode, image is always at 0x8000, so we should also move it from
 * there.
 *
 * General idea of this code:
 * - Detect current scheme (bootloader or no-bootloader, current boot address)
 * - If current schema is correct, just return
 * - Move code to unused OTA slot to avoid overwriting code that is being
 * executed
 * - Mark current slot as unused
 * - Reboot into the new slot
 *
 * Note that this code runs in ram-code section, because in bootloader mode
 * flash-resident code cannot be used (it was linked to run from different
 * offset). So, we cannot do any printf and have to use only ram-code versions
 * of flash functions.
 */
#include "ota_reformating/ensure_ota_scheme.h"

#include "ota_reformating/ram_code_flash.h"

#pragma pack(push, 1)
#include "chip_8258/flash.h"
#include "tl_common.h"
#pragma pack(pop)

// Ramcode versions of flash read/write functions

int _attribute_ram_code_sec_ ram_code_memcmp(const void *m1, const void *m2,
                                             unsigned int n) {
  unsigned char *s1 = (unsigned char *)m1;
  unsigned char *s2 = (unsigned char *)m2;

  while (n--) {
    if (*s1 != *s2) {
      return *s1 - *s2;
    }
    s1++;
    s2++;
  }
  return 0;
}

bool _attribute_ram_code_sec_ is_valid_image_at_address(u32 addr) {
  // Check if the image at the given address is valid
  u32 flashInfo = 0;
  ram_code_flash_read_page(addr + FLASH_TLNK_FLAG_OFFSET, 4, (u8 *)&flashInfo);
  return (flashInfo == TL_START_UP_FLAG_WHOLE);
}

u32 _attribute_ram_code_sec_ get_firmware_size(u32 addr) {
  u32 size = 0;
  ram_code_flash_read_page(addr + IMAGE_SIZE_OFFSET, 4, (u8 *)&size);
  return size;
}

void _attribute_ram_code_sec_ move_flash_data(u32 from, u32 to, u32 size) {
  u32 bufCache[256 / 4]; // align to 4, not sure
  u8 *buf = (u8 *)bufCache;
  u8 verifyBuf[256];

  for (int i = 0; i < size; i += 256) {
    if ((i & 0xfff) == 0) {
      ram_code_flash_erase_sector(to + i);
    }

    ram_code_flash_read_page(from + i, 256, buf);

    ram_code_flash_write_page(to + i, 256, buf);

    ram_code_flash_read_page(to + i, 256, verifyBuf);
    if (ram_code_memcmp(verifyBuf, buf, 256)) {
      SYSTEM_RESET(); // Verification failed, reset
    }
  }
}

void _attribute_ram_code_sec_ ensure_correct_ota_scheme(void) {

  u32 current_addr = 0x0;
  u32 copy_to_addr = 0x0;

  bool uses_bootloader = is_valid_image_at_address(BOOTLOADER_MODE_MAIN_ADDR);

  if (uses_bootloader) {
    current_addr = BOOTLOADER_MODE_MAIN_ADDR;
    copy_to_addr = FLASH_ADDR_OF_OTA_IMAGE;
  } else {
    if (is_valid_image_at_address(SMALL_OTA_FLASH_ADDR)) {
      current_addr = SMALL_OTA_FLASH_ADDR;
      copy_to_addr = FLASH_ADDR_OF_OTA_IMAGE;
    } else {
      return; // We booted either from 0x00000 or 0x40000, which is correct
    }
  }

  u32 firmware_size = get_firmware_size(current_addr);
  if (firmware_size == 0 || firmware_size > MAX_FIRMWARE_SIZE) {
    SYSTEM_RESET(); // Invalid firmware size, reset
  }

  ram_code_flash_write_status(FLASH_TYPE_8BIT_STATUS,
                              0); // Disable write protection

  move_flash_data(current_addr, copy_to_addr, firmware_size);
  // Mark old location as unused

  u32 unused_flag = 0x0;
  if (uses_bootloader) {
    // Mark both bootloader and bootloader-mode main address as unused
    ram_code_flash_write_page(0x0 + FLASH_TLNK_FLAG_OFFSET, 4,
                              (u8 *)&unused_flag);
    ram_code_flash_write_page(BOOTLOADER_MODE_MAIN_ADDR +
                                  FLASH_TLNK_FLAG_OFFSET,
                              4, (u8 *)&unused_flag);
  } else {
    // Mark small-OTA slot as unused
    ram_code_flash_write_page(SMALL_OTA_FLASH_ADDR + FLASH_TLNK_FLAG_OFFSET, 4,
                              (u8 *)&unused_flag);
  }
  SYSTEM_RESET();
}