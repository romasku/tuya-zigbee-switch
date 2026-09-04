#ifndef _DP_ATTR_H_
#define _DP_ATTR_H_

#include "hal/zigbee.h"
#include <stdint.h>

/* Generic bridge between Tuya datapoints and custom ZCL attributes.
 *
 * Settings-style datapoints (backlight, indicator, vibration, ...) have no
 * standard ZCL equivalent, so each one is published as a manufacturer specific
 * attribute on the Basic cluster of the first endpoint.
 *
 * The mapping is runtime data, exactly like the pin configuration: it comes
 * from a second config string stored in NV and exposed as attribute 0xff11.
 * Keeping it out of the main config string is not a style choice, the ZCL
 * write path caps a single string at ~74 characters.
 *
 * Format, 6 characters per entry:
 *
 *     <dp:2hex><type:1><attr_low:2hex>;
 *
 *     type      B = bool, E = enum, V = value (4 byte big endian on the wire)
 *     attr_low  low byte of the attribute id, always inside 0xff00..0xffff
 *
 * Example, the eight datapoints of the Moes SFL02-Z-2 / Nova Digital TPZ-2:
 *
 *     12E20;13E21;24B22;25E23;67B24;68E25;69V26;6AV27;
 */

#define MAX_DP_ATTRS    12

typedef struct {
    uint8_t  dp_id;    /* Tuya datapoint id */
    uint8_t  dp_type;  /* TUYA_DP_TYPE_BOOL / _ENUM / _VALUE */
    uint16_t attr_id;  /* ZCL attribute id */
    uint8_t  zcl_type; /* ZCL_DATA_TYPE_BOOLEAN / _ENUM8 / _UINT32 */
    uint8_t  zcl_size; /* 1 or 4 */
    /* Value in ZCL layout (little endian). The attribute table points here, so
     * it must stay put for the lifetime of the device. */
    uint8_t  raw[4];
} dp_attr_t;

extern dp_attr_t dp_attrs[MAX_DP_ATTRS];
extern uint8_t   dp_attrs_cnt;

/** Parse the dp config string into dp_attrs. Resets any previous content. */
void    dp_attr_parse(const char *str, uint16_t len);

/** Append every parsed attribute to a cluster attribute table.
 *  Returns how many entries were written. */
uint8_t dp_attr_register(hal_zigbee_attribute *table, uint8_t start_index);

/** Hook the datapoint report callback chain. Call once, after the other
 *  consumers of the chain have registered. */
void    dp_attr_init(void);

/** Push the current value of an attribute down to the secondary MCU.
 *  Called from the Basic cluster write callback. Returns 1 if handled. */
uint8_t dp_attr_on_write(uint16_t attr_id);

/** True for a short window after a query-all. The MCU answers it by
 *  dumping every datapoint, including the ones that carry key state, so
 *  consumers must not mistake that burst for physical presses. */
uint8_t dp_attr_bulk_dump_active(void);

/** Ask the MCU to report every datapoint it knows about. */
void    dp_attr_query_all(void);

#endif
