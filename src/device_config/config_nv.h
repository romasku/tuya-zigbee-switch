#ifndef _CONFIG_NV_H_
#define _CONFIG_NV_H_

#include <stdint.h>

// Following structure (2 byte length, data follows) is ZCL LONG_STRING format.
// This way it allows us to use it directly inside Basic cluster
#define DEVICE_CONFIG_MAX_LEN    127 // characters; data[] also holds the closing 0

// The Zigbee stack copies a written string into this buffer without knowing its capacity, so a
// too long write would run over whatever follows. `overflow_guard` takes such a write; the write
// handler then rejects it. Only the first DEVICE_CONFIG_NV_SIZE bytes are stored, so the stored
// item keeps its old size.
typedef struct {
    uint16_t size;
    uint8_t  data[DEVICE_CONFIG_MAX_LEN + 1];
    uint8_t  overflow_guard[256];
} device_config_str_t;

#define DEVICE_CONFIG_NV_SIZE    (sizeof(uint16_t) + DEVICE_CONFIG_MAX_LEN + 1)

extern device_config_str_t device_config_str;

void device_config_write_to_nv();
void device_config_remove_from_nv();
void device_config_read_from_nv();

void handle_version_changes();

#endif
