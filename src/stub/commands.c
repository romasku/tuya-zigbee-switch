#include "commands.h"
#include "machine_io.h"
#include "parsing.h"

#include "hal/timer.h"
#include "hal/zigbee.h"

#include "stub/hal/stub.h"

#include "stub/stub_app.h"
#include "zigbee/consts.h"
#include <stdio.h>
#include <string.h>

extern volatile sig_atomic_t g_should_exit;

static int cmd_machine(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: machine on|off\n");
    io_res_err("usage");
    return -1;
  }
  if (strcmp(argv[1], "on") == 0) {
    g_machine_mode = true;
    io_res_ok(NULL);
    return 0;
  }
  if (strcmp(argv[1], "off") == 0) {
    io_res_ok(NULL);
    g_machine_mode = false;
    fprintf(stderr, "machine: off\n");
    return 0;
  }
  io_res_err("arg");
  return -1;
}

static int cmd_help(int argc, char **argv) {
  (void)argc;
  (void)argv;
  stub_app_print_help();
  io_res_ok(NULL);
  return 0;
}
static int cmd_status(int argc, char **argv) {
  (void)argc;
  (void)argv;
  stub_app_show_status();
  io_res_ok("uptime_ms=%u joined=%d", hal_millis(),
            hal_zigbee_get_network_status());
  return 0;
}
static int cmd_quit(int argc, char **argv) {
  (void)argc;
  (void)argv;
  g_should_exit = 1;
  io_res_ok(NULL);
  return 0;
}
static int cmd_net(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: net <0|1|2>\n");
    io_res_err("usage");
    return -1;
  }
  char *e = NULL;
  long val = strtol(argv[1], &e, 10);
  if (*argv[1] == '\0' || *e || (val < 0 || val > 2)) {
    fprintf(stderr,
            "Value must be 0 (NOT_JOINED), 1 (JOINED), or 2 (JOINING)\n");
    io_res_err("bad_value=%s", argv[1]);
    return -1;
  }
  stub_zigbee_set_network_status((int)val);
  const char *status_str;
  switch ((hal_zigbee_network_status_t)val) {
  case HAL_ZIGBEE_NETWORK_NOT_JOINED:
    status_str = "NOT_JOINED";
    break;
  case HAL_ZIGBEE_NETWORK_JOINED:
    status_str = "JOINED";
    break;
  case HAL_ZIGBEE_NETWORK_JOINING:
    status_str = "JOINING";
    break;
  default:
    status_str = "UNKNOWN";
    break;
  }
  printf("Network status: %s\n", status_str);
  io_res_ok("joined=%ld status=%s", val, status_str);
  return 0;
}
static int cmd_pin(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: p <pin> <0|1>\n");
    io_res_err("usage");
    return -1;
  }
  char *e = NULL;
  long pin = strtol(argv[1], &e, 10);
  if (*argv[1] == '\0' || *e) {
    fprintf(stderr, "Bad pin\n");
    io_res_err("bad_pin=%s", argv[1]);
    return -1;
  }
  long val = strtol(argv[2], &e, 10);
  if (*argv[2] == '\0' || *e || (val != 0 && val != 1)) {
    fprintf(stderr, "Value must be 0 or 1\n");
    io_res_err("bad_value=%s", argv[2]);
    return -1;
  }
  stub_gpio_simulate_input((int)pin, (int)val);
  printf("Pin %ld <= %ld\n", pin, val);
  io_res_ok("pin=%ld value=%ld", pin, val);
  return 0;
}

static int cmd_zcl_list_attrs(int argc, char **argv) {
  (void)argc;
  (void)argv;
  stub_app_list_attrs();
  io_res_ok(NULL);
  return 0;
}

static int cmd_zcl_read(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: zcl_read <ep:dec> <cluster:hex> <attr:hex>\n");
    io_res_err("usage");
    return -1;
  }
  uint8_t ep;
  uint16_t cl, at;
  if (parse_u8_dec(argv[1], &ep) || parse_u16_hex(argv[2], &cl) ||
      parse_u16_hex(argv[3], &at)) {
    fprintf(stderr, "Bad args\n");
    io_res_err("bad_args");
    return -1;
  }
  hal_zigbee_attribute *attr = stub_app_find_attr(ep, cl, at);
  if (attr) {
    char buf[256];
    const char *val_str =
        stub_app_attribute_value_to_string(attr, buf, sizeof(buf));
    stub_app_print_attribute_value(attr);
    io_res_ok("ep=%d cluster=0x%04X attr=0x%04X value=%s", ep, cl, at, val_str);
  } else {
    puts("Attribute not found");
    io_res_err("attr_not_found ep=%d cluster=0x%04X attr=0x%04X", ep, cl, at);
  }
  return 0;
}

static int cmd_zcl_write(int argc, char **argv) {
  if (argc != 5) {
    fprintf(stderr,
            "Usage: zcl_write <ep:dec> <cluster:hex> <attr:hex> <value>\n");
    io_res_err("usage");
    return -1;
  }
  uint8_t ep;
  uint16_t cl, at;
  if (parse_u8_dec(argv[1], &ep) || parse_u16_hex(argv[2], &cl) ||
      parse_u16_hex(argv[3], &at)) {
    fprintf(stderr, "Bad args\n");
    io_res_err("bad_args");
    return -1;
  }

  // Find attribute to determine its type
  hal_zigbee_attribute *attr = stub_app_find_attr(ep, cl, at);
  if (!attr) {
    fprintf(stderr,
            "Attribute not found (ep=%u, cluster=0x%04X, attr=0x%04X)\n", ep,
            cl, at);
    io_res_err("attr_not_found ep=%u cluster=0x%04X attr=0x%04X", ep, cl, at);
    return -1;
  }

  int ret = stub_app_string_to_attribute_value(attr, argv[4]);
  if (ret != 0) {
    fprintf(stderr, "Failed to parse value for attribute (code %d)\n", ret);
    io_res_err("bad_value ep=%u cluster=0x%04X attr=0x%04X", ep, cl, at);
    return -1;
  }

  stub_simulate_zigbee_attribute_write(ep, cl, at);
  io_res_ok("ep=%d cluster=0x%04X attr=0x%04X value=%s", ep, cl, at, argv[4]);

  return 0;
}

static int cmd_read_pin(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: read_pin <pin>\n");
    io_res_err("usage");
    return -1;
  }
  char *e = NULL;
  int pin = strtol(argv[1], &e, 10);
  if (*argv[1] == '\0' || *e) {
    fprintf(stderr, "Bad pin\n");
    io_res_err("bad_pin=%s", argv[1]);
    return -1;
  }
  int val = stub_gpio_get_output(pin);
  printf("Pin %d => %d\n", pin, val);
  io_res_ok("pin=%d value=%d", pin, val);
  return 0;
}

static int cmd_zcl_cmd(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Usage: zcl_cmd <ep:dec> <cluster:hex> <cmd:hex> "
                    "[payload:hex_bytes...]\n");
    io_res_err("usage");
    return -1;
  }

  uint8_t ep;
  uint16_t cluster, cmd_id;

  if (parse_u8_dec(argv[1], &ep) || parse_u16_hex(argv[2], &cluster) ||
      parse_u16_hex(argv[3], &cmd_id)) {
    fprintf(stderr, "Bad args\n");
    io_res_err("bad_args");
    return -1;
  }

  // Parse optional payload bytes
  uint8_t payload[64] = {0}; // Max payload size
  int payload_len = 0;

  for (int i = 4; i < argc && payload_len < sizeof(payload); i++) {
    char *e = NULL;
    long byte_val = strtol(argv[i], &e, 16);
    if (*argv[i] == '\0' || *e || byte_val < 0 || byte_val > 0xFF) {
      fprintf(stderr, "Bad payload byte: %s\n", argv[i]);
      io_res_err("bad_payload_byte=%s", argv[i]);
      return -1;
    }
    payload[payload_len++] = (uint8_t)byte_val;
  }

  hal_zigbee_cmd_result_t result = stub_zigbee_simulate_command(
      ep, cluster, cmd_id, payload_len > 0 ? payload : NULL);

  const char *result_str;
  switch (result) {
  case HAL_ZIGBEE_CMD_PROCESSED:
    result_str = "PROCESSED";
    break;
  case HAL_ZIGBEE_CMD_SKIPPED:
    result_str = "SKIPPED";
    break;
  default:
    result_str = "UNKNOWN";
    break;
  }

  printf("ZCL command result: %s (ep=%d, cluster=0x%04X, cmd=0x%02X, "
         "payload_len=%d)\n",
         result_str, ep, cluster, cmd_id, payload_len);
  io_res_ok("ep=%d cluster=0x%04X cmd=0x%02X result=%s payload_len=%d", ep,
            cluster, cmd_id, result_str, payload_len);

  return 0;
}

static int cmd_freeze_time(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: freeze_time <0|1>\n");
    io_res_err("usage");
    return -1;
  }
  int freeze = atoi(argv[1]);
  if (freeze != 0 && freeze != 1) {
    fprintf(stderr, "Invalid argument: %s\n", argv[1]);
    io_res_err("bad_args");
    return -1;
  }
  if (freeze) {
    stub_millis_freeze();
  } else {
    stub_millis_unfreeze();
  }
  io_res_ok("freeze_time=%d", freeze);
  return 0;
}

static int cmd_step_time(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: step_time <milliseconds>\n");
    io_res_err("usage");
    return -1;
  }
  char *e = NULL;
  long step = strtol(argv[1], &e, 10);
  if (*argv[1] == '\0' || *e || step < 0) {
    fprintf(stderr, "Bad step value: %s\n", argv[1]);
    io_res_err("bad_step=%s", argv[1]);
    return -1;
  }
  stub_millis_step((uint64_t)step);
  io_res_ok("stepped_ms=%ld", step);
  return 0;
}

/* Command table */
static const SimpleReplCommand kCmds[] = {
    {"machine", cmd_machine},
    {"help", cmd_help},
    {"s", cmd_status},
    {"status", cmd_status},
    {"net", cmd_net},
    {"set_pin", cmd_pin},
    {"read_pin", cmd_read_pin},
    {"zcl_read", cmd_zcl_read},
    {"zcl_write", cmd_zcl_write},
    {"zcl_list_attrs", cmd_zcl_list_attrs},
    {"zcl_cmd", cmd_zcl_cmd},
    {"freeze_time", cmd_freeze_time},
    {"step_time", cmd_step_time},
    {"q", cmd_quit},
    {"quit", cmd_quit},
};

const SimpleReplCommand *commands_table(void) { return kCmds; }
size_t commands_count(void) { return sizeof(kCmds) / sizeof(kCmds[0]); }

void commands_print_help(void) { stub_app_print_help(); }
