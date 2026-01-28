#ifndef ENSURE_OTA_SCHEME_H
#define ENSURE_OTA_SCHEME_H

#include "tl_common.h"

#define FLASH_TLNK_FLAG_OFFSET       8
#define IMAGE_SIZE_OFFSET            0x18
#define TL_START_UP_FLAG_WHOLE       0x544c4e4b

#define BOOTLOADER_MODE_MAIN_ADDR    0x8000
#define SMALL_OTA_FLASH_ADDR         0x20000
#define BIG_OTA_FLASH_ADDR           0x40000
#define MAX_FIRMWARE_SIZE            0x40000

void _attribute_ram_code_sec_ ensure_correct_ota_scheme(void);

#endif
