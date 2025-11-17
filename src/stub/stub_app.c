#include "stub_app.h"
#include "app.h"
#include <stdio.h>
#include <string.h>

#include "base_components/button.h"
#include "base_components/led.h"
#include "base_components/network_indicator.h"
#include "base_components/relay.h"
#include "device_config/config_nv.h"
#include "device_config/config_parser.h"
#include "device_config/nvm_items.h"
#include "hal/gpio.h"
#include "hal/nvm.h"
#include "hal/system.h"
#include "hal/timer.h"
#include "hal/zigbee.h"
#include "stub/hal/stub.h"
#include "stub/machine_io.h"
#include "zigbee/basic_cluster.h"
#include "zigbee/consts.h"
#include "zigbee/general_commands.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/switch_cluster.h"

// externs from your codebase
extern led_t leds[5];
extern uint8_t leds_cnt;
extern button_t buttons[5];
extern uint8_t buttons_cnt;
extern relay_t relays[5];
extern uint8_t relays_cnt;

static device_config_str_t g_stub_config = {
    .size = 0, .data = "Stub;Stub;SA0u;SA1u;SA2u;SA3u;RB0;RB1;RC0;RC1;"};

void stub_app_init(const char *device_conf, bool joined) {
  puts("[STUB] Starting Smart Home Device Stub");

  bool nvm_has_config = false;
  if (hal_nvm_read(NV_ITEM_DEVICE_CONFIG, sizeof(device_config_str_t),
                   (uint8_t *)&g_stub_config) == HAL_NVM_SUCCESS) {
    nvm_has_config = true;
  }

  if (device_conf) {
    snprintf((char *)g_stub_config.data, sizeof(g_stub_config.data), "%s",
             device_conf);
  }
  if (device_conf || !nvm_has_config) {
    printf("[STUB] Using device configuration: %s\n", g_stub_config.data);
    g_stub_config.size = (uint16_t)strnlen((const char *)g_stub_config.data,
                                           sizeof(g_stub_config.data));
    hal_nvm_write(NV_ITEM_DEVICE_CONFIG, sizeof(device_config_str_t),
                  (uint8_t *)&g_stub_config);
  }

  puts("[STUB] Initializing stub application");
  // buttons released
  stub_gpio_simulate_input(hal_gpio_parse_pin("A0"), 1);
  stub_gpio_simulate_input(hal_gpio_parse_pin("A1"), 1);
  stub_gpio_simulate_input(hal_gpio_parse_pin("A2"), 1);
  stub_gpio_simulate_input(hal_gpio_parse_pin("A3"), 1);

  if (joined) {
    stub_zigbee_set_network_status(HAL_ZIGBEE_NETWORK_JOINED);
  }

  app_init();

  puts("[STUB] Application initialized");
}

void stub_app_shutdown() {
  puts("[STUB] Cleaning up...");
  stub_zigbee_clear_bindings();
  puts("[STUB] Shutdown complete");
}

void stub_app_poll() {
  app_task();
  stub_tasks_poll();
}

void stub_app_print_help(void) {
  puts("Stub Smart Home Device Simulator");
  puts("Commands:");
  puts("  machine on|off                        - Set machine mode");
  puts("  help                                  - Show this help");
  puts("  s, status                             - Show device status");
  puts(
      "  net                                   - Toggle network joined status");
  puts("  set_pin <pin> <0|1>                   - Simulate GPIO input");
  puts("  read_pin <pin>                        - Read GPIO output");
  puts("  zcl_list_attrs                        - List all Zigbee attributes");
  puts("  zcl_read <ep> <cluster> <attr>        - Read attribute (ep dec, IDs "
       "hex)");
  puts("  zcl_write <ep> <cluster> <attr> <v>   - Write attribute (v dec/hex)");
  puts("  zcl_cmd <ep> <cluster> <cmd> [bytes]  - Simulate ZCL command (hex "
       "bytes)");
  puts("  freeze_time <0|1>                     - Freeze/unfreeze time");
  puts("  step_time <ms>                        - Advance time by ms");
  puts("  q, quit                               - Exit");
}

void stub_app_show_status(void) {
  printf("\n=== Device Status ===\n");
  for (uint8_t i = 0; i < leds_cnt; i++) {
    printf("LED %u: %s (pin %d = %d)\n", (unsigned)(i + 1),
           leds[i].on ? "ON" : "OFF", leds[i].pin,
           stub_gpio_get_output(leds[i].pin));
  }
  for (uint8_t i = 0; i < relays_cnt; i++) {
    printf("Relay %u: %s (pin %d = %d)\n", (unsigned)i,
           relays[i].on ? "ON" : "OFF", relays[i].pin,
           stub_gpio_get_output(relays[i].pin));
  }
  for (uint8_t i = 0; i < buttons_cnt; i++) {
    const char *state = (buttons[i].pressed && buttons[i].long_pressed)
                            ? "LONG PRESSED"
                        : buttons[i].pressed ? "PRESSED"
                                             : "RELEASED";
    printf("Button %u: %s (pin %d state: %d)\n", (unsigned)i, state,
           buttons[i].pin, stub_gpio_get_output(buttons[i].pin));
  }
  if (hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED) {
    puts("Network: JOINED\n");
  } else if (hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINING) {
    puts("Network: JOINING\n");
  } else {
    puts("Network: NOT JOINED\n");
  }
  printf("Uptime: %u ms\n", hal_millis());
  puts("=== Device Status End ===\n");
}

hal_zigbee_attribute *stub_app_find_attr(uint8_t ep, uint16_t cluster,
                                         uint16_t attr) {
  uint8_t cnt = 0;
  hal_zigbee_endpoint *eps = stub_zigbee_get_endpoints(&cnt);
  return hal_zigbee_find_attribute(eps, cnt, ep, cluster, attr);
}

const char *stub_app_attribute_value_to_string(hal_zigbee_attribute *attr,
                                               char *buf, size_t bufsize) {
  if (!attr || !attr->value || bufsize == 0) {
    if (bufsize > 0)
      buf[0] = '\0';
    return buf;
  }

  switch (attr->data_type_id) {
  case ZCL_DATA_TYPE_BOOLEAN:
  case ZCL_DATA_TYPE_UINT8:
  case ZCL_DATA_TYPE_ENUM8:
    if (attr->size >= 1)
      snprintf(buf, bufsize, "%u", attr->value[0]);
    else
      buf[0] = '\0';
    break;
  case ZCL_DATA_TYPE_UINT16:
    if (attr->size >= 2) {
      uint16_t val = attr->value[0] | (attr->value[1] << 8);
      snprintf(buf, bufsize, "%u", val);
    } else {
      buf[0] = '\0';
    }
    break;
  case ZCL_DATA_TYPE_CHAR_STR: {
    if (attr->size >= 1) {
      uint8_t len = attr->value[0];
      if (len > attr->size - 1)
        len = attr->size - 1;
      snprintf(buf, bufsize, "%.*s", len, (char *)&attr->value[1]);
    } else {
      buf[0] = '\0';
    }
    break;
  }
  case ZCL_DATA_TYPE_LONG_CHAR_STR: {
    if (attr->size >= 2) {
      uint16_t len = (uint16_t)attr->value[0] | ((uint16_t)attr->value[1] << 8);
      if (len > attr->size - 2)
        len = attr->size - 2;
      snprintf(buf, bufsize, "%.*s", (int)len, (char *)&attr->value[2]);
    } else {
      buf[0] = '\0';
    }
    break;
  }
  default: {
    size_t n = 0;
    for (int i = 0; i < attr->size && n + 3 < bufsize; i++) {
      n += snprintf(buf + n, bufsize - n, "%02x ", attr->value[i]);
    }
    if (n > 0 && n < bufsize)
      buf[n - 1] = '\0'; // remove trailing space
    else if (bufsize > 0)
      buf[0] = '\0';
    break;
  }
  }
  return buf;
}

int stub_app_string_to_attribute_value(hal_zigbee_attribute *attr,
                                       const char *str) {
  if (!attr || !attr->value || !str)
    return -1;

  if (attr->flag == ATTR_READONLY) {
    io_log("ZIGBEE", "Warning: Writing to read-only attribute\n");
  }

  switch (attr->data_type_id) {
  case ZCL_DATA_TYPE_BOOLEAN:
  case ZCL_DATA_TYPE_UINT8:
  case ZCL_DATA_TYPE_ENUM8: {
    unsigned int v = 0;
    if (sscanf(str, "%u", &v) != 1)
      return -2;
    if (attr->size >= 1)
      attr->value[0] = (uint8_t)v;
    else
      return -3;
    break;
  }
  case ZCL_DATA_TYPE_UINT16: {
    unsigned int v = 0;
    if (sscanf(str, "%u", &v) != 1)
      return -2;
    if (attr->size >= 2) {
      attr->value[0] = (uint8_t)(v & 0xFF);
      attr->value[1] = (uint8_t)((v >> 8) & 0xFF);
    } else
      return -3;
    break;
  }
  case ZCL_DATA_TYPE_CHAR_STR: {
    size_t len = strlen(str);
    if (attr->size < 1)
      return -3;
    if (len > attr->size - 1)
      len = attr->size - 1;
    attr->value[0] = (uint8_t)len;
    memcpy(&attr->value[1], str, len);
    break;
  }
  case ZCL_DATA_TYPE_LONG_CHAR_STR: {
    size_t len = strlen(str);
    if (attr->size < 2)
      return -3;
    if (len > attr->size - 2)
      len = attr->size - 2;
    attr->value[0] = (uint8_t)(len & 0xFF);
    attr->value[1] = (uint8_t)((len >> 8) & 0xFF);
    memcpy(&attr->value[2], str, len);
    break;
  }
  default: {
    // Parse as hex bytes: "01 02 03"
    size_t n = 0;
    const char *p = str;
    while (*p && n < attr->size) {
      unsigned int v;
      int consumed = 0;
      if (sscanf(p, "%2x%n", &v, &consumed) == 1 && consumed > 0) {
        attr->value[n++] = (uint8_t)v;
        p += consumed;
        while (*p == ' ' || *p == ',')
          p++;
      } else {
        break;
      }
    }
    if (n < attr->size) {
      // Zero the rest
      memset(attr->value + n, 0, attr->size - n);
    }
    break;
  }
  }
  return 0;
}

void stub_app_print_attribute_value(hal_zigbee_attribute *attr) {
  if (!attr || !attr->value) {
    printf("NULL attribute or value\n");
    return;
  }

  printf("Attr 0x%04x (type 0x%02x, size %d): ", attr->attribute_id,
         attr->data_type_id, attr->size);

  char buf[128];
  printf("%s\n", stub_app_attribute_value_to_string(attr, buf, sizeof(buf)));
}

void stub_app_list_attrs(void) {
  uint8_t endpoints_count = 0;
  hal_zigbee_endpoint *endpoints = stub_zigbee_get_endpoints(&endpoints_count);
  if (!endpoints) {
    printf("No Zigbee endpoints found\n");
    return;
  }

  printf("\n=== Zigbee Attributes ===\n");

  for (int i = 0; i < endpoints_count; i++) {
    printf("Endpoint %d:\n", endpoints[i].endpoint);

    for (int j = 0; j < endpoints[i].cluster_count; j++) {
      hal_zigbee_cluster *cluster = &endpoints[i].clusters[j];
      printf("  Cluster 0x%04x (%s):\n", cluster->cluster_id,
             cluster->is_server ? "server" : "client");

      for (int k = 0; k < cluster->attribute_count; k++) {
        hal_zigbee_attribute *attr = &cluster->attributes[k];
        printf("    ");
        stub_app_print_attribute_value(attr);
      }
    }
  }
  printf("========================\n\n");
}
