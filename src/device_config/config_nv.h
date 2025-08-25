#ifndef _CONFIG_NV_H_
#define _CONFIG_NV_H_


// Following structure (2 byte length, data follows) is ZCL LONG_STRING format.
// This way it allows us to use it directly inside Basic cluster
typedef struct {
  u16  size;
  char data[256];
} device_config_str_t;

extern device_config_str_t device_config_str;  

void device_config_write_to_nv();
void device_config_remove_from_nv();
void device_config_read_from_nv();

#endif
