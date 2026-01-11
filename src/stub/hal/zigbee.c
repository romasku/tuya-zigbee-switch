#include "hal/zigbee.h"
#include "stub/machine_io.h"
#include "stub/parsing.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENDPOINTS 16
#define MAX_BINDINGS 32

typedef struct {
  uint16_t short_addr;
  uint8_t endpoint;
  uint16_t cluster_id;
} stub_binding_t;

static hal_zigbee_endpoint *endpoints = NULL;
static uint8_t endpoints_count = 0;
static hal_zigbee_network_status_t network_status =
    HAL_ZIGBEE_NETWORK_NOT_JOINED;
static hal_attribute_change_callback_t attr_change_callback = NULL;

static stub_binding_t bindings[MAX_BINDINGS];
static int binding_count = 0;

void hal_zigbee_init(hal_zigbee_endpoint *ep_list, uint8_t ep_count) {
  if (!ep_list && ep_count > 0) {
    io_log("ZIGBEE", "Error: NULL endpoint list with non-zero count %d",
           ep_count);
    exit(1);
  }

  if (ep_count > MAX_ENDPOINTS) {
    io_log("ZIGBEE", "Error: Endpoint count %d exceeds maximum %d", ep_count,
           MAX_ENDPOINTS);
    exit(1);
  }

  endpoints = ep_list;
  endpoints_count = ep_count;

  io_log("ZIGBEE", "Initialized Zigbee with %d endpoints", ep_count);

  for (int i = 0; i < ep_count; i++) {
    io_log("ZIGBEE", "Endpoint %d: profile=0x%04x, device=0x%04x, clusters=%d",
           ep_list[i].endpoint, ep_list[i].profile_id, ep_list[i].device_id,
           ep_list[i].cluster_count);

    if (ep_list[i].cluster_count > 0 && !ep_list[i].clusters) {
      io_log(
          "ZIGBEE",
          "Error: Endpoint %d has cluster count %d but NULL clusters pointer",
          ep_list[i].endpoint, ep_list[i].cluster_count);
      exit(1);
    }
    for (int j = 0; j < ep_list[i].cluster_count; j++) {
      io_log("ZIGBEE", "  Cluster %d: id=0x%04x, attrs=%d, is_server=%d", j,
             ep_list[i].clusters[j].cluster_id,
             ep_list[i].clusters[j].attribute_count,
             ep_list[i].clusters[j].is_server);
      if (ep_list[i].clusters[j].attribute_count > 0 &&
          !ep_list[i].clusters[j].attributes) {
        io_log("ZIGBEE",
               "Error: Endpoint %d Cluster %d has attribute count %d but NULL "
               "attributes pointer",
               ep_list[i].endpoint, j, ep_list[i].clusters[j].attribute_count);
        exit(1);
      }
      for (int k = 0; k < ep_list[i].clusters[j].attribute_count; k++) {
        io_log("ZIGBEE",
               "    Attr %d: id=0x%04x, type=0x%02x, size=%d, flags=%d", k,
               ep_list[i].clusters[j].attributes[k].attribute_id,
               ep_list[i].clusters[j].attributes[k].data_type_id,
               ep_list[i].clusters[j].attributes[k].size,
               ep_list[i].clusters[j].attributes[k].flag);
      }
    }
  }
}

hal_zigbee_network_status_t hal_zigbee_get_network_status(void) {
  return network_status;
}

static hal_network_status_change_callback_t network_status_change_callback =
    NULL;

void hal_register_on_network_status_change_callback(
    hal_network_status_change_callback_t callback) {
  network_status_change_callback = callback;
}

void hal_zigbee_leave_network(void) {
  if (network_status == HAL_ZIGBEE_NETWORK_NOT_JOINED) {
    io_log("ZIGBEE", "Cannot leave network - not joined");
    return;
  }
  io_evt("zcl_leave_network");
  io_log("ZIGBEE", "Leaving network");
  network_status = HAL_ZIGBEE_NETWORK_NOT_JOINED;
  if (network_status_change_callback != NULL) {
    network_status_change_callback(hal_zigbee_get_network_status());
  }
}

void hal_zigbee_start_network_steering() {
  io_log("ZIGBEE", "Starting network steering (joining)");
  io_evt("zcl_start_network_steering");
  network_status = HAL_ZIGBEE_NETWORK_JOINING;
  if (network_status_change_callback != NULL) {
    network_status_change_callback(hal_zigbee_get_network_status());
  }
}

void hal_zigbee_notify_attribute_changed(uint8_t endpoint, uint16_t cluster_id,
                                         uint16_t attribute_id) {
  io_log("ZIGBEE", "Attribute changed: ep=%d, cluster=0x%04x, attr=0x%04x",
         endpoint, cluster_id, attribute_id);
  hal_zigbee_attribute *attr = hal_zigbee_find_attribute(
      endpoints, endpoints_count, endpoint, cluster_id, attribute_id);
  if (!attr) {
    io_log("ZIGBEE",
           "Error: Notified about change of unregistered attribute "
           "not found for ep=%d, cluster=0x%04x, attr=0x%04x",
           endpoint, cluster_id, attribute_id);

    // TODO: Fix this properly
    // exit(1);
  }
  io_evt("zcl_attr_change ep=%u cluster=0x%04X attr=0x%04X", endpoint,
         cluster_id, attribute_id);
}

void hal_zigbee_register_on_attribute_change_callback(
    hal_attribute_change_callback_t callback) {
  attr_change_callback = callback;
  io_log("ZIGBEE", "Registered attribute change callback");
}

hal_zigbee_status_t hal_zigbee_send_cmd_to_bindings(const hal_zigbee_cmd *cmd) {
  if (!cmd)
    return HAL_ZIGBEE_ERR_BAD_ARG;

  if (network_status != HAL_ZIGBEE_NETWORK_JOINED) {
    io_log("ZIGBEE", "Cannot send command - not joined to network");
    return HAL_ZIGBEE_ERR_NOT_JOINED;
  }

  io_log("ZIGBEE", "Sending command: ep=%d, cluster=0x%04x, cmd=0x%02x, len=%d",
         cmd->endpoint, cmd->cluster_id, cmd->command_id, cmd->payload_len);

  char buffer[cmd->payload_len * 2 + 1];
  bytes_to_hexstr(cmd->payload, cmd->payload_len, buffer);

  io_evt("zcl_cmd_send ep=%u cluster=0x%04X cmd=0x%02X len=%u data_hex=%s",
         cmd->endpoint, cmd->cluster_id, cmd->command_id, cmd->payload_len,
         buffer);

  // Simulate sending to all relevant bindings
  int sent_count = 0;
  for (int i = 0; i < binding_count; i++) {
    if (bindings[i].endpoint == cmd->endpoint &&
        bindings[i].cluster_id == cmd->cluster_id) {
      io_log("ZIGBEE", "Sent to binding %d (addr=0x%04x)", i,
             bindings[i].short_addr);
      sent_count++;
    }
  }

  if (sent_count == 0) {
    io_log("ZIGBEE", "No matching bindings found");
  }

  return HAL_ZIGBEE_OK;
}

hal_zigbee_status_t
hal_zigbee_send_report_attr(uint8_t endpoint, uint16_t cluster_id,
                            uint16_t attr_id, uint8_t zcl_type_id,
                            const void *value, uint8_t value_len) {
  if (!value)
    return HAL_ZIGBEE_ERR_BAD_ARG;

  if (network_status != HAL_ZIGBEE_NETWORK_JOINED) {
    io_log("ZIGBEE", "Cannot send report - not joined to network");
    return HAL_ZIGBEE_ERR_NOT_JOINED;
  }

  io_log("ZIGBEE",
         "Sending attribute report: ep=%d, cluster=0x%04x, attr=0x%04x, "
         "type=0x%02x, len=%d",
         endpoint, cluster_id, attr_id, zcl_type_id, value_len);

  return HAL_ZIGBEE_OK;
}

hal_zigbee_status_t hal_zigbee_send_announce(void) {
  io_log("ZIGBEE", "Sending Zigbee announce");
  io_evt("zdo_announce");
  return HAL_ZIGBEE_OK;
}

void stub_zigbee_set_network_status(hal_zigbee_network_status_t status) {
  network_status = status;
  io_log("ZIGBEE", "Network status set to %d", network_status);
  if (network_status_change_callback != NULL) {
    network_status_change_callback(hal_zigbee_get_network_status());
  }
}

void stub_zigbee_add_binding(uint16_t short_addr, uint8_t endpoint,
                             uint16_t cluster_id) {
  if (binding_count >= MAX_BINDINGS) {
    io_log("ZIGBEE", "Cannot add binding - table full");
    return;
  }

  bindings[binding_count].short_addr = short_addr;
  bindings[binding_count].endpoint = endpoint;
  bindings[binding_count].cluster_id = cluster_id;
  binding_count++;

  io_log("ZIGBEE", "Added binding: addr=0x%04x, ep=%d, cluster=0x%04x",
         short_addr, endpoint, cluster_id);
}

void stub_zigbee_clear_bindings(void) {
  binding_count = 0;
  io_log("ZIGBEE", "Cleared all bindings");
}

hal_zigbee_endpoint *stub_zigbee_get_endpoints(uint8_t *count) {
  if (count)
    *count = endpoints_count;
  return endpoints;
}

// Simulate receiving a ZCL command
hal_zigbee_cmd_result_t stub_zigbee_simulate_command(uint8_t endpoint,
                                                     uint16_t cluster_id,
                                                     uint8_t command_id,
                                                     void *payload) {
  io_log("ZIGBEE", "Simulating command: ep=%d, cluster=0x%04x, cmd=0x%02x",
         endpoint, cluster_id, command_id);

  // Find the endpoint and cluster
  for (int i = 0; i < endpoints_count; i++) {
    if (endpoints[i].endpoint == endpoint) {
      for (int j = 0; j < endpoints[i].cluster_count; j++) {
        hal_zigbee_cluster *cluster = &endpoints[i].clusters[j];
        if (cluster->cluster_id == cluster_id && cluster->cmd_callback) {
          return cluster->cmd_callback(endpoint, cluster_id, command_id,
                                       payload);
        }
      }
    }
  }

  return HAL_ZIGBEE_CMD_SKIPPED;
}

void stub_simulate_zigbee_attribute_write(uint8_t endpoint, uint16_t cluster_id,
                                          uint16_t attribute_id) {

  if (attr_change_callback) {
    attr_change_callback(endpoint, cluster_id, attribute_id);
  }
}