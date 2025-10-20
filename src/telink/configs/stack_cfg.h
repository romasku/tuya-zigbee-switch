#ifndef _STACK_CFG_H_
#define _STACK_CFG_H_

/*
 * Basic Stack Configuration
 */

/* Working channel (valid range: 11-26) */
#define DEFAULT_CHANNEL 11

/* Enable NVRAM storage */
#define NV_ENABLE 1

/* Enable security features */
#define SECURITY_ENABLE 1

/*
 * Application-Specific Parameters
 * Adjust these values according to application requirements
 */

/* ZCL: Maximum number of clusters (in + out cluster count) */
#define ZCL_CLUSTER_NUM_MAX 32

/* ZCL: Maximum number of reporting table entries */
#define ZCL_REPORTING_TABLE_NUM 12

/* ZCL: Maximum number of scene table entries */
#define ZCL_SCENE_TABLE_NUM 8

/* APS: Maximum number of group entries (8 endpoints per group) */
#define APS_GROUP_TABLE_NUM 8

/* APS: Maximum number of binding table entries */
#define APS_BINDING_TABLE_NUM 32

/*
 * Automatic Configuration
 * The following settings are calculated automatically based on role
 */

/* Automatic role definition based on device type */
#if (COORDINATOR)
#define ZB_ROUTER_ROLE 1
#define ZB_COORDINATOR_ROLE 1
#elif (ROUTER)
#define ZB_ROUTER_ROLE 1
#elif (END_DEVICE)
#define ZB_ED_ROLE 1
#endif

/*
 * Power Management Configuration
 * If PM_ENABLE is set, ZB_MAC_RX_ON_WHEN_IDLE must be 0
 */
#if ZB_ED_ROLE
#if PM_ENABLE
#define ZB_MAC_RX_ON_WHEN_IDLE 0 /* Required for power management */
#endif

#ifndef ZB_MAC_RX_ON_WHEN_IDLE
#define ZB_MAC_RX_ON_WHEN_IDLE 0 /* Default: receiver off when idle */
#endif
#endif

#endif /* _STACK_CFG_H_ */
