#ifndef _CONFIG_NV_H_
#define _CONFIG_NV_H_

#include <stdint.h>

// Following structure (2 byte length, data follows) is ZCL LONG_STRING format.
// This way it allows us to use it directly inside Basic cluster
typedef struct {
    uint16_t size;
    uint8_t  data[128];
} device_config_str_t;

extern device_config_str_t device_config_str;

/* Second config string, holding the Tuya datapoint to ZCL attribute map.
 * Split from the main one because a single ZCL write caps at ~74 chars. */
extern device_config_str_t dp_config_str;

/* Overflow for device_config itself: the same ~74-char single-write limit
 * applies to it, and a config string with many peripheral tokens (e.g. a
 * board declaring six switch and six relay endpoints) can run past that.
 * Read and appended onto device_config_str in RAM by
 * device_config_append_ext(), before parse_config()'s token loop runs, so
 * the result is identical to one long string -- only the over-the-air
 * write needs to happen in two parts. */
extern device_config_str_t device_config_ext_str;

void device_config_write_to_nv();
void device_config_remove_from_nv();
void device_config_read_from_nv();

void dp_config_write_to_nv();
void dp_config_read_from_nv();

void device_config_ext_write_to_nv();
void device_config_ext_read_from_nv();
/* Reads device_config_ext_str from NV and appends it onto device_config_str
 * (clamped to the latter's buffer capacity), re-terminating the combined
 * string. No-op if device_config_ext_str is empty. Must run after
 * device_config_read_from_nv() and before parse_config() tokenizes
 * device_config_str. */
void device_config_append_ext();

void handle_version_changes();

#endif
