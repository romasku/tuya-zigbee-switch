#ifndef _VERSION_CFG_H_
#define _VERSION_CFG_H_

/*
 * Boot Loader Configuration
 */
#define BOOT_LOADER_MODE          0   /* Boot loader mode: 0=disabled, 1=enabled */
#define BOOT_LOADER_IMAGE_ADDR    0x0 /* Boot loader flash address */

/* Application image address based on boot loader mode */
#if (BOOT_LOADER_MODE)
#define APP_IMAGE_ADDR    0x8000 /* App starts at 32KB when bootloader enabled */
#else
#define APP_IMAGE_ADDR    0x0    /* App starts at 0x0 when no bootloader */
#endif

/*
 * Board Identification
 */
#define BOARD_TS0012    0x01 /* 2-gang switch board */
#define BOARD_TS0001    0x02 /* 1-gang switch board */
#define BOARD_TS0002    0x03 /* 2-gang switch board (alt) */
#define BOARD_TS0011    0x04 /* 1-gang switch board (alt) */

#ifndef BOARD
#define BOARD           BOARD_TS0012 /* Default board type */
#endif

/*
 * Firmware Version Information
 */
#define FIRMWARE_TYPE_PREFIX        0xaa /* Firmware type identifier */

/*
 * OTA Update Configuration
 * These values are checked during OTA upgrades according to ZCL OTA
 * specification
 */
#define MANUFACTURER_CODE_TELINK    0x1141 /* Telink manufacturer ID */

#ifndef IMAGE_TYPE
#define IMAGE_TYPE                  43521 /* OTA image type identifier */
#endif

/* Combined file version for OTA (32-bit): APP_REL | APP_BUILD | STACK_REL |
 * STACK_BUILD */
#ifndef FILE_VERSION
#define FILE_VERSION    0
#endif

/*
 * Linker Configuration
 */
#define IS_BOOT_LOADER_IMAGE          0              /* This is not a bootloader image */
#define RESV_FOR_APP_RAM_CODE_SIZE    0x0            /* Reserved RAM for application code */
#define IMAGE_OFFSET                  APP_IMAGE_ADDR /* Image offset in flash */

#endif /* _VERSION_CFG_H_ */
