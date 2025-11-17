#ifndef STUB_APP_H
#define STUB_APP_H

#include "hal/zigbee.h"
#include <stdint.h>

#define APP_DEVICE_CONF_MAX 256

/* lifecycle */
void stub_app_init(const char *device_conf_or_null, bool joined);
void stub_app_shutdown(void);

/* polling (1ms cadence via REPL) */
void stub_app_poll(void);

/* UI helpers */
void stub_app_print_help(void);
void stub_app_show_status(void);

/* Zigbee helpers */
hal_zigbee_attribute *stub_app_find_attr(uint8_t ep, uint16_t cluster,
                                         uint16_t attr);
void stub_app_list_attrs(void);

void stub_app_print_attribute_value(hal_zigbee_attribute *attr);
const char *stub_app_attribute_value_to_string(hal_zigbee_attribute *attr,
                                               char *buf, size_t bufsize);
int stub_app_string_to_attribute_value(hal_zigbee_attribute *attr,
                                       const char *str);

#endif
