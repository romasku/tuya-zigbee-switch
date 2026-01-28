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

void device_config_write_to_nv();
void device_config_remove_from_nv();
void device_config_read_from_nv();

void handle_version_changes();

#endif
