#ifndef ZCL_BASIC_CONFIG_H
#define ZCL_BASIC_CONFIG_H



/*********************************************************************
 * CONSTANTS
 */

/**
 *  @brief	relative humidity measurement cluster Attribute IDs
 */

#define ZCL_ATTRID_ONOFF_INDICATOR_MODE      0xff01
#define ZCL_ATTRID_ONOFF_INDICATOR_STATE     0xff02

#ifdef INDICATOR_PWM_SUPPORT
#define ZCL_ATTRID_ONOFF_INDICATOR_DIMMING_MODE       0xff03
#define ZCL_ATTRID_ONOFF_INDICATOR_DIMMING_BRIGHTNESS 0xff04
#endif

#define ZCL_ONOFF_INDICATOR_MODE_SAME        0x00
#define ZCL_ONOFF_INDICATOR_MODE_OPPOSITE    0x01
#define ZCL_ONOFF_INDICATOR_MODE_MANUAL      0x02


#endif  /* ZCL_RELATIVE_HUMIDITY_H */
