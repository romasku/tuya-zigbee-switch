#ifndef _APP_CFG_H_
#define _APP_CFG_H_

/*
 * Version Configuration
 */
#include "version_cfg.h"

/*
 * Product Configuration
 */

/* Debug Output Configuration */
#ifndef UART_PRINTF_MODE
#define UART_PRINTF_MODE 0 /* UART debug output mode */
/* UART Configuration */
#endif
#define DEBUG_BAUDRATE 115200      /* UART baud rate */
#define DEBUG_INFO_TX_PIN GPIO_PB1 /* Debug output pin */

/* Power Management */
#ifndef PM_ENABLE
#define PM_ENABLE 0 /* Power management enable */
#endif

/* Power Amplifier */
#define PA_ENABLE 0 /* External power amplifier enable */

/* System Clock Configuration */
#define CLOCK_SYS_CLOCK_HZ 24000000 /* System clock frequency (24MHz) */

/* RF Configuration */
#define RF_TX_POWER_DEF RF_POWER_P10p46dBm /* Default TX power */

/* Module Enables */
#define MODULE_WATCHDOG_ENABLE 0 /* Watchdog module */
#define MODULE_UART_ENABLE 0     /* UART module */

/* Host Controller Interface */
#if (ZBHCI_USB_PRINT || ZBHCI_USB_CDC || ZBHCI_USB_HID || ZBHCI_UART)
#define ZBHCI_EN 1
#endif

/*
 * ZCL Cluster Support Configuration
 */
#define ZCL_POWER_CFG_SUPPORT 0          /* Power configuration cluster */
#define ZCL_ON_OFF_SUPPORT 1             /* On/Off cluster */
#define ZCL_ONOFF_CONFIGUATION 1         /* On/Off cluster configuration */
#define ZCL_LEVEL_CTRL 1                 /* Level control cluster */
/* #define ZCL_IAS_ZONE_SUPPORT 1     */ /* IAS Zone cluster (disabled) */
#define ZCL_POLL_CTRL_SUPPORT 0          /* Poll control cluster */
#define ZCL_GROUP_SUPPORT 1              /* Groups cluster */
#define ZCL_OTA_SUPPORT 1                /* OTA upgrade cluster */

/* Zigbee Features */
#define TOUCHLINK_SUPPORT 0     /* TouchLink commissioning */
#define FIND_AND_BIND_SUPPORT 0 /* Find and bind feature */
#define REJOIN_FAILURE_TIMER 1  /* Rejoin failure timer */
#define GP_SUPPORT_ENABLE 1     /* Green Power support */

/* Hardware Configuration */
#define VOLTAGE_DETECT_ADC_PIN 0 /* ADC pin for voltage detection */

/*
 * Stack Configuration Files
 */
#include "stack_cfg.h"
#include "zb_config.h"

/*
 * Event Configuration
 */
typedef enum {
  EV_POLL_ED_DETECT, /* End device detection polling */
  EV_POLL_HCI,       /* Host controller interface polling */
  EV_POLL_IDLE,      /* Idle state polling */
  EV_POLL_MAX,
} ev_poll_e;

#endif /* _APP_CFG_H_ */
