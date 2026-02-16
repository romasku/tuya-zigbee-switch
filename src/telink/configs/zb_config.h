#ifndef ZB_CONFIG_H
#define ZB_CONFIG_H

#include "tl_common.h"

/*
 * Zigbee Stack Common Configuration
 */

/* Stack Profile */
#define ZB_STACK_PROFILE          2 /* Zigbee PRO stack profile */

/* Frame Type Definitions */
#define ZB_FRAME_TYPE_MASK        0x03 /* Frame type field mask */
#define ZB_PROTOCOL_VER_MASK      0x3c /* Protocol version field mask */
#define ZB_PROTOCOL_VER_POS       2    /* Protocol version field position */

/* Protocol Versions */
#define ZB_PROTOCOL_VERSION       2 /* Standard Zigbee protocol version */
#define ZB_PROTOCOL_VERSION_GP    3 /* Green Power protocol version */

/* MAC Frame Version */
#define MAC_FRAME_VERSION \
        MAC_FRAME_IEEE_802_15_4_2003 /* Default MAC frame version */

/*
 * Network Address Assignment
 * Note: Only stochastic address assignment is used for ZigBee PRO
 */
#ifndef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
#define ZB_NWK_STOCHASTIC_ADDRESS_ASSIGN    /* Use stochastic addressing (ZigBee \
                                             * PRO) */
#endif

/*
 * ZDO (Zigbee Device Object) Configuration
 */

/* ZDO Default Configuration Attributes */
#define ZDO_PERMIT_JOIN_DURATION \
        0 /* Duration to permit joining (0 = disabled) */

/* Polling Configuration (all values in milliseconds) */
#define POLL_RATE_QUARTERSECONDS  250 /* 1 quarter-second = 250ms (SDK constant) */
#ifdef BATTERY_POWERED
#define POLL_RATE_MS              120000 /* Battery: poll every 120s */
#else
#define POLL_RATE_MS              1000   /* Router: poll every 1s */
#endif
#define POLL_NO_DATA_MAX_COUNT    3 /* Max consecutive polls with no data */

/* Network Discovery Configuration */
#define ZDO_NWK_SCAN_ATTEMPTS \
        5   /* Number of scan attempts before association (1-255) */
#define ZDO_NWK_TIME_BTWN_SCANS \
        100 /* Time between scan attempts in ms (1-65535) */

/* Indirect Polling Configuration */
#define ZDO_NWK_INDIRECT_POLL_RATE    1000 /* 1000ms */

/* Parent Link Configuration */
#define ZDO_MAX_PARENT_THRESHOLD_RETRY \
        5 /* Max retry attempts for parent link */

/* Rejoin Configuration */
#define ZDO_REJOIN_TIMES                5  /* Number of rejoin attempts */
#define ZDO_REJOIN_DURATION             6  /* Rejoin duration in seconds */
#define ZDO_REJOIN_BACKOFF_TIME         30 /* Initial backoff time in seconds */
#define ZDO_MAX_REJOIN_BACKOFF_TIME     90 /* Maximum backoff time in seconds */
#define ZDO_REJOIN_BACKOFF_ITERATION    8  /* Number of backoff iterations */

/*
 * Role-Specific Configuration
 */
#if ZB_ROUTER_ROLE
#if ZB_COORDINATOR_ROLE
#define NWK_ROUTE_RECORD_TABLE_NUM \
        127                           /* Route record table size (coordinator) */
#endif
#define TL_ZB_NWK_ADDR_MAP_NUM    128 /* Network address mapping table size */
#define ROUTING_TABLE_NUM         48  /* Routing table size */
#endif

/*
 * Polling Rate Configuration
 */
#define POLL_RATE          POLL_RATE_MS              /* Normal poll rate (ms) */
#ifdef BATTERY_POWERED
#define RESPONSE_POLL_RATE 250                       /* Battery: 250ms when coordinator has data */
#define QUEUE_POLL_RATE    250                       /* Battery: 250ms when outbound queued */
#else
#define RESPONSE_POLL_RATE POLL_RATE_QUARTERSECONDS  /* Router: 250ms */
#define QUEUE_POLL_RATE    POLL_RATE_QUARTERSECONDS  /* Router: 250ms */
#endif
#define REJOIN_POLL_RATE   (2 * POLL_RATE_QUARTERSECONDS) /* Rejoin: 500ms */

/*
 * Security Configuration
 */
#define CCM_KEY_SIZE             16 /* AES-CCM key size (fixed) */
#define SECUR_N_SECUR_MATERIAL \
        2                           /* Number of NWK keys to maintain (NLS5 requirement) */
#define ZB_CCM_M                 4  /* CCM M parameter for security level 5 */
#define ZB_SECURITY              1  /* Enable Zigbee security */
#define APS_FRAME_SECURITY          /* Enable APS layer security */

/* Stack Profile Implementation */
#define ZB_STACK_PROFILE_2007    /* Enable ZigBee 2007 stack profile */

/*
 * Feature Configuration
 */
#if ZB_ROUTER_ROLE
#define GP_SUPPORT_ENABLE    1 /* Enable Green Power support for routers */
#endif

/* Default RF Configuration */
#ifdef BATTERY_POWERED
#define ZB_DEFAULT_TX_POWER_IDX \
        RF_POWER_INDEX_P3p01dBm  /* Battery: reduced TX power (~3dBm) for lower consumption */
#else
#define ZB_DEFAULT_TX_POWER_IDX \
        RF_POWER_INDEX_P10p46dBm /* Router: maximum TX power */
#endif

#endif /* ZB_CONFIG_H */
