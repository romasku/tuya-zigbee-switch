/********************************************************************************************************
 * @file    board_cgdk2.h
 *
 * @brief   This is the header file for board_cgdk2
 *
 *******************************************************************************************************/
#ifndef _BOARD_CGDK2_H_
#define _BOARD_CGDK2_H_

#include "version_cfg.h"

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif


#define ZCL_MANUFACTURER    "Tuya-TS0002-custom"
#define ZCL_MODEL   "TS0002-custom"

#define RF_TX_POWER_DEF RF_POWER_P10p46dBm


// LED
#define GPIO_LED			GPIO_PC2
#define LED_ON 				1
#define LED_OFF				0

#define PC2_INPUT_ENABLE	0
#define PC2_DATA_OUT		0
#define PC2_OUTPUT_ENABLE	1
#define PC2_FUNC			AS_GPIO

// Button on board
#define ON_BOARD_BUTTON	    GPIO_PD2
#define PD2_INPUT_ENABLE	1
#define PD2_DATA_OUT		0
#define PD2_OUTPUT_ENABLE	0
#define PD2_FUNC			AS_GPIO
#define PULL_WAKEUP_SRC_PD2 PM_PIN_PULLUP_10K

// Button S1
#define S1_BUTTON	        GPIO_PB5
#define PB5_INPUT_ENABLE	1
#define PB5_DATA_OUT		0
#define PB5_OUTPUT_ENABLE	0
#define PB5_FUNC			AS_GPIO
#define PULL_WAKEUP_SRC_PB5 PM_PIN_PULLUP_10K

// Button S2
#define S2_BUTTON	        GPIO_PB4
#define PB4_INPUT_ENABLE	1
#define PB4_DATA_OUT		0
#define PB4_OUTPUT_ENABLE	0
#define PB4_FUNC			AS_GPIO
#define PULL_WAKEUP_SRC_PB4 PM_PIN_PULLUP_10K


enum{
    KEY_ON_BOARD = 0x01,
    KEY_S1 = 0x02,
    KEY_S2 = 0x03
};


#define KB_MAP_NORMAL   {\
        {KEY_ON_BOARD,}, \
        {KEY_S1,}, \
        {KEY_S2,}, }

#define KB_MAP_NUM      KB_MAP_NORMAL
#define KB_MAP_FN       KB_MAP_NORMAL

#define KB_DRIVE_PINS  {NULL }
#define KB_SCAN_PINS   {ON_BOARD_BUTTON,  S1_BUTTON, S2_BUTTON}


// Control S1
#define GPIO_S1_PWR 	    GPIO_PC4
#define PC4_INPUT_ENABLE	0
#define PC4_DATA_OUT		0
#define PC4_OUTPUT_ENABLE	1
#define PC4_FUNC			AS_GPIO

// Control S2
#define GPIO_S2_PWR 	    GPIO_PC3
#define PC3_INPUT_ENABLE	0
#define PC3_DATA_OUT		0
#define PC3_OUTPUT_ENABLE	1
#define PC3_FUNC			AS_GPIO

#define PWR_ON              1
#define PWR_OFF             0


#define BAUDRATE            115200
#define	DEBUG_INFO_TX_PIN	GPIO_PB1 //print


// Enable tuya ota

#define ZIGBEE_TUYA_OTA 	1


#define SWITCH_CLUSTERS 2
#define RELAY_CLUSTERS 2


/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
#endif // (BOARD == BOARD_CGDK2)