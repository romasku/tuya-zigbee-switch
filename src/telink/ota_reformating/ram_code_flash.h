#ifndef RAM_CODE_FLASH_H_
#define RAM_CODE_FLASH_H_

#include "chip_8258/flash.h"
#include "tl_common.h"

/** RAM code version of flash operations
 *
 * These functions allow to perform flash operations in ram-code only
 * environment. This is used when reformatting from bootloader mode to no
 * bootloader mode, as in this case we cannot use flash-resident code as it was
 * linked to run from differerent offset.
 */

void _attribute_ram_code_sec_ ram_code_flash_read_page(unsigned long addr,
                                                       unsigned long len,
                                                       unsigned char *buf);

void _attribute_ram_code_sec_ ram_code_flash_write_page(unsigned long addr,
                                                        unsigned long len,
                                                        unsigned char *buf);

void _attribute_ram_code_sec_ ram_code_flash_erase_sector(unsigned long addr);
void _attribute_ram_code_sec_
ram_code_flash_write_status(flash_status_typedef_e type, unsigned short data);

#endif /* RAM_CODE_FLASH_H_ */
