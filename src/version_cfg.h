/********************************************************************************************************
 * @file    version_cfg.h
 *
 * @brief   This is the header file for version_cfg
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *			All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#ifndef _VERSION_CFG_H_
#define _VERSION_CFG_H_

#define BOOT_LOADER_MODE          0

/* Boot loader address. */
#define BOOT_LOADER_IMAGE_ADDR    0x0

/* APP image address. */
#if (BOOT_LOADER_MODE)
        #define APP_IMAGE_ADDR    0x8000
#else
        #define APP_IMAGE_ADDR    0x0
#endif

/* Board ID */
#define BOARD_TS0012    0x01
#define BOARD_TS0001    0x02
#define BOARD_TS0002    0x03
#define BOARD_TS0011    0x04


/* Board define */
#ifndef BOARD
#define BOARD                   BOARD_TS0012
#endif

#define FIRMWARE_TYPE_PREFIX    0xaa


#define APP_RELEASE             0x02            //BCD app release "0.1"
#define APP_BUILD               0x03            //BCD app build "0.2"
#define STACK_RELEASE           0x30            //BCD stack release 3.0

#ifndef STACK_BUILD
#define STACK_BUILD             0x00            //BCD stack build 01
#endif

/*********************************************************************************************
 * During OTA upgrade, the upgraded device will check the rules of the following three fields.
 * Refer to ZCL OTA specification for details.
 */
#define MANUFACTURER_CODE_TELINK      0x1141 // Telink ID
#ifndef IMAGE_TYPE
#define IMAGE_TYPE                    43521
#endif
#define FILE_VERSION                  ((APP_RELEASE << 24) | (APP_BUILD << 16) | (STACK_RELEASE << 8) | STACK_BUILD)

/* Pre-compiled link configuration. */
#define IS_BOOT_LOADER_IMAGE          0
#define RESV_FOR_APP_RAM_CODE_SIZE    0
#define IMAGE_OFFSET                  APP_IMAGE_ADDR

#endif // _VERSION_CFG_H_
