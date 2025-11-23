#ifndef ZIGBEE_CONSTS_H_
#define ZIGBEE_CONSTS_H_


#define ZCL_HA_PROFILE 0x0104

// Clusters

#define ZCL_CLUSTER_BASIC 0
#define ZCL_CLUSTER_ON_OFF 6
#define ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG  7
#define ZCL_CLUSTER_MULTISTATE_INPUT_BASIC            0x0012
#define ZCL_CLUSTER_LEVEL_CONTROL                     0x0008
#define ZCL_CLUSTER_GROUPS                            0x0004
#define ZCL_CLUSTER_OTA_BOOTLOAD                      0x0019


// Attributes

// Global

#define ZCL_ATTR_GLOBAL_CLUSTER_REVISION                0xFFFD

// Basic cluster

#define ZCL_ATTR_BASIC_ZCL_VER                          0x0000
#define ZCL_ATTR_BASIC_APP_VER                          0x0001
#define ZCL_ATTR_BASIC_STACK_VER                        0x0002
#define ZCL_ATTR_BASIC_HW_VER                           0x0003
#define ZCL_ATTR_BASIC_MFR_NAME                         0x0004
#define ZCL_ATTR_BASIC_MODEL_ID                         0x0005
#define ZCL_ATTR_BASIC_DATE_CODE                        0x0006
#define ZCL_ATTR_BASIC_POWER_SOURCE                     0x0007
#define ZCL_ATTR_BASIC_LOC_DESC                         0x0010
#define ZCL_ATTR_BASIC_PHY_ENV                          0x0011
#define ZCL_ATTR_BASIC_DEV_ENABLED                      0x0012
#define ZCL_ATTR_BASIC_ALARM_MASK                       0x0013
#define ZCL_ATTR_BASIC_DISABLE_LOCAL_CFG                0x0014
#define ZCL_ATTR_BASIC_SW_BUILD_ID                      0x4000

#define ZCL_ATTR_BASIC_DEVICE_CONFIG                    0xff00
#define ZCL_ATTR_BASIC_STATUS_LED_STATE                 0xff01

// OnOff cluster

#define ZCL_ATTR_ONOFF                                  0x0000
#define ZCL_ATTR_START_UP_ONOFF                         0x4003

#define ZCL_ATTR_ONOFF_INDICATOR_MODE                   0xff01
#define ZCL_ATTR_ONOFF_INDICATOR_STATE                  0xff02

#define ZCL_ATTR_ONOFF_DETACHED_RELAY_MODE              0xff03  //todo: setup attribute + nv + zha

// OnOff configuration cluster

#define ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE                  0x0000
#define ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS               0x0010
#define ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE                  0xff00
#define ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE            0xff01
#define ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX           0xff02
#define ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR        0xff03
#define ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE       0xff04
#define ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE          0xff05


// Multistate cluster

#define ZCL_ATTR_MULTISTATE_INPUT_NUMBER_OF_STATES    0x004A
#define ZCL_ATTR_MULTISTATE_INPUT_OUT_OF_SERVICE      0x0051
#define ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE       0x0055
#define ZCL_ATTR_MULTISTATE_INPUT_STATUS_FLAGS        0x006F


// Groups cluster

#define ZCL_ATTR_GROUP_NAME_SUPPORT                     0x0000

// OTA cluster

#define ZCL_ATTR_OTA_UPGRADE_SERVER_ID                  0x0000
#define ZCL_ATTR_OTA_FILE_OFFSET                        0x0001
#define ZCL_ATTR_OTA_CURRENT_FILE_VERSION               0x0002
#define ZCL_ATTR_OTA_CURRENT_ZIGBEE_STACK_VERSION       0x0003
#define ZCL_ATTR_OTA_DOWNLOADED_FILE_VERSION            0x0004
#define ZCL_ATTR_OTA_DOWNLOADED_ZIGBEE_STACK_VERSION    0x0005
#define ZCL_ATTR_OTA_IMAGE_UPGRADE_STATUS               0x0006
#define ZCL_ATTR_OTA_MANUFACTURER_ID                    0x0007
#define ZCL_ATTR_OTA_IMAGE_TYPE_ID                      0x0008
#define ZCL_ATTR_OTA_MINIMUM_BLOCK_PERIOD               0x0009
#define ZCL_ATTR_OTA_IMAGE_STAMP                        0x000A

// Attr values

// OnOff cluster

#define ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF             0x00
#define ZCL_START_UP_ONOFF_SET_ONOFF_TO_ON              0x01
#define ZCL_START_UP_ONOFF_SET_ONOFF_TOGGLE             0x02
#define ZCL_START_UP_ONOFF_SET_ONOFF_TO_PREVIOUS        0xFF

#define ZCL_ONOFF_INDICATOR_MODE_SAME                   0x00
#define ZCL_ONOFF_INDICATOR_MODE_OPPOSITE               0x01
#define ZCL_ONOFF_INDICATOR_MODE_MANUAL                 0x02

#define ZCL_ONOFF_RELAY_MODE_ATTACHED                   0x00 //todo: setup attribute + nv + zha
#define ZCL_ONOFF_RELAY_MODE_DETACHED                   0x01 //todo: setup attribute + nv + zha

// OnOff configuration cluster

#define ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE                  0x00
#define ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY               0x01
#define ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY_NC            0x02

#define ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF                 0x00
#define ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON                 0x01
#define ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE         0x02
#define ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC     0x03
#define ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE 0x04

#define ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED                 0x00
#define ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE                     0x01
#define ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG                     0x02
#define ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT                    0x03

#define ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE                     0x01
#define ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG                     0x02
#define ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT                    0x03


// Level cluster

#define ZCL_LEVEL_MOVE_UP                                     0x00
#define ZCL_LEVEL_MOVE_DOWN                                   0x01

// Commands

// OnOff Cluster

#define ZCL_CMD_ONOFF_OFF                               0x00
#define ZCL_CMD_ONOFF_ON                                0x01
#define ZCL_CMD_ONOFF_TOGGLE                            0x02
#define ZCL_CMD_OFF_WITH_EFFECT                         0x40
#define ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE             0x41
#define ZCL_CMD_ON_WITH_TIMED_OFF                       0x42

// Level Cluster

#define ZCL_CMD_LEVEL_MOVE_TO_LEVEL                       0x00
#define ZCL_CMD_LEVEL_MOVE                                0x01
#define ZCL_CMD_LEVEL_STEP                                0x02
#define ZCL_CMD_LEVEL_STOP                                0x03
#define ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF           0x04
#define ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF                    0x05
#define ZCL_CMD_LEVEL_STEP_WITH_ON_OFF                    0x06
#define ZCL_CMD_LEVEL_STOP_WITH_ON_OFF                    0x07

// OTA Cluster

#define ZCL_CMD_OTA_IMAGE_NOTIFY                          0x00
#define ZCL_CMD_OTA_QUERY_NEXT_IMAGE_REQUEST              0x01
#define ZCL_CMD_OTA_QUERY_NEXT_IMAGE_RESPONSE             0x02
#define ZCL_CMD_OTA_IMAGE_BLOCK_REQUEST                   0x03
#define ZCL_CMD_OTA_IMAGE_PAGE_REQUEST                    0x04
#define ZCL_CMD_OTA_IMAGE_BLOCK_RESPONSE                  0x05
#define ZCL_CMD_OTA_UPGRADE_END_REQUEST                   0x06
#define ZCL_CMD_OTA_UPGRADE_END_RESPONSE                  0x07
#define ZCL_CMD_OTA_QUERY_DEVICE_SPECIFIC_FILE_REQUEST    0x08
#define ZCL_CMD_OTA_QUERY_DEVICE_SPECIFIC_FILE_RESPONSE   0x09

// Data types

#define ZCL_DATA_TYPE_NO_DATA                           0x00
#define ZCL_DATA_TYPE_DATA8                             0x08
#define ZCL_DATA_TYPE_DATA16                            0x09
#define ZCL_DATA_TYPE_DATA24                            0x0a
#define ZCL_DATA_TYPE_DATA32                            0x0b
#define ZCL_DATA_TYPE_DATA40                            0x0c
#define ZCL_DATA_TYPE_DATA48                            0x0d
#define ZCL_DATA_TYPE_DATA56                            0x0e
#define ZCL_DATA_TYPE_DATA64                            0x0f
#define ZCL_DATA_TYPE_BOOLEAN                           0x10
#define ZCL_DATA_TYPE_BITMAP8                           0x18
#define ZCL_DATA_TYPE_BITMAP16                          0x19
#define ZCL_DATA_TYPE_BITMAP24                          0x1a
#define ZCL_DATA_TYPE_BITMAP32                          0x1b
#define ZCL_DATA_TYPE_BITMAP40                          0x1c
#define ZCL_DATA_TYPE_BITMAP48                          0x1d
#define ZCL_DATA_TYPE_BITMAP56                          0x1e
#define ZCL_DATA_TYPE_BITMAP64                          0x1f
#define ZCL_DATA_TYPE_UINT8                             0x20
#define ZCL_DATA_TYPE_UINT16                            0x21
#define ZCL_DATA_TYPE_UINT24                            0x22
#define ZCL_DATA_TYPE_UINT32                            0x23
#define ZCL_DATA_TYPE_UINT40                            0x24
#define ZCL_DATA_TYPE_UINT48                            0x25
#define ZCL_DATA_TYPE_UINT56                            0x26
#define ZCL_DATA_TYPE_UINT64                            0x27
#define ZCL_DATA_TYPE_INT8                              0x28
#define ZCL_DATA_TYPE_INT16                             0x29
#define ZCL_DATA_TYPE_INT24                             0x2a
#define ZCL_DATA_TYPE_INT32                             0x2b
#define ZCL_DATA_TYPE_INT40                             0x2c
#define ZCL_DATA_TYPE_INT48                             0x2d
#define ZCL_DATA_TYPE_INT56                             0x2e
#define ZCL_DATA_TYPE_INT64                             0x2f
#define ZCL_DATA_TYPE_ENUM8                             0x30
#define ZCL_DATA_TYPE_ENUM16                            0x31
#define ZCL_DATA_TYPE_SEMI_PREC                         0x38
#define ZCL_DATA_TYPE_SINGLE_PREC                       0x39
#define ZCL_DATA_TYPE_DOUBLE_PREC                       0x3a
#define ZCL_DATA_TYPE_OCTET_STR                         0x41
#define ZCL_DATA_TYPE_CHAR_STR                          0x42
#define ZCL_DATA_TYPE_LONG_OCTET_STR                    0x43
#define ZCL_DATA_TYPE_LONG_CHAR_STR                     0x44
#define ZCL_DATA_TYPE_ARRAY                             0x48
#define ZCL_DATA_TYPE_STRUCT                            0x4c
#define ZCL_DATA_TYPE_SET                               0x50
#define ZCL_DATA_TYPE_BAG                               0x51
#define ZCL_DATA_TYPE_TOD                               0xe0
#define ZCL_DATA_TYPE_DATE                              0xe1
#define ZCL_DATA_TYPE_UTC                               0xe2
#define ZCL_DATA_TYPE_CLUSTER_ID                        0xe8
#define ZCL_DATA_TYPE_ATTR_ID                           0xe9
#define ZCL_DATA_TYPE_BAC_OID                           0xea
#define ZCL_DATA_TYPE_IEEE_ADDR                         0xf0
#define ZCL_DATA_TYPE_128_BIT_SEC_KEY                   0xf1
#define ZCL_DATA_TYPE_UNKNOWN                           0xff



#endif /* ZIGBEE_CONSTS_H_ */
