#include "hal/zigbee.h"

#include "app/framework/include/af.h"
#include "app/framework/plugin/ota-client/ota-client.h"
#include "network-steering.h"
#include <stddef.h>
#include <string.h>

#define MAX_CLUSTERS 32
#define MAX_ATTRS 128

sl_zigbee_af_endpoint_type_t endpoint_type_buffer[ZCL_FIXED_ENDPOINT_COUNT];
sl_zigbee_af_cluster_t clusters_buffer[MAX_CLUSTERS];
sl_zigbee_af_attribute_metadata_t attributes_buffer[MAX_ATTRS];

hal_zigbee_endpoint *hal_endpoints;
uint8_t hal_endpoints_cnt;

hal_zigbee_cluster *find_hal_cluster(uint8_t endpoint,
                                     sl_zigbee_af_cluster_id_t clusterId) {
  return hal_zigbee_find_cluster(hal_endpoints, hal_endpoints_cnt, endpoint,
                                 clusterId);
}

hal_zigbee_attribute *find_hal_attr(uint8_t endpoint,
                                    sl_zigbee_af_cluster_id_t clusterId,
                                    sl_zigbee_af_attribute_id_t attributeId) {
  return hal_zigbee_find_attribute(hal_endpoints, hal_endpoints_cnt, endpoint,
                                   clusterId, attributeId);
}

static uint32_t on_command_callback(sl_service_opcode_t opcode,
                                    sl_service_function_context_t *context) {
  assert(opcode == SL_SERVICE_FUNCTION_TYPE_ZCL_COMMAND);

  sl_zigbee_af_cluster_command_t *cmd =
      (sl_zigbee_af_cluster_command_t *)context->data;
  hal_zigbee_cluster *hal_cluster = find_hal_cluster(
      cmd->apsFrame->destinationEndpoint, cmd->apsFrame->clusterId);
  if (hal_cluster == NULL || hal_cluster->cmd_callback == NULL)
    return SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND;
  hal_zigbee_cmd_result_t res = hal_cluster->cmd_callback(
      cmd->apsFrame->destinationEndpoint, cmd->apsFrame->clusterId,
      cmd->commandId, cmd->buffer);
  if (res == HAL_ZIGBEE_CMD_PROCESSED) {
    sl_zigbee_af_send_immediate_default_response(SL_ZIGBEE_ZCL_STATUS_SUCCESS);
    return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
  }
  return SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND;
}

void hal_zigbee_init(hal_zigbee_endpoint *endpoints, uint8_t endpoints_cnt) {
  hal_endpoints = endpoints;
  hal_endpoints_cnt = endpoints_cnt;

  for (int i = 0; i < ZCL_FIXED_ENDPOINT_COUNT; i++) {
    sl_zigbee_af_endpoint_enable_disable(sli_zigbee_af_endpoints[i].endpoint,
                                         false);
  }

  // Avoid settings more endpoints then device supports
  endpoints_cnt = endpoints_cnt <= ZCL_FIXED_ENDPOINT_COUNT
                      ? endpoints_cnt
                      : ZCL_FIXED_ENDPOINT_COUNT;

  sl_zigbee_af_endpoint_type_t *endpoint_type_ptr = endpoint_type_buffer;
  sl_zigbee_af_cluster_t *cluster_ptr = clusters_buffer;
  sl_zigbee_af_attribute_metadata_t *attr_ptr = attributes_buffer;

  for (int i = 0; i < endpoints_cnt; i++) {
    sli_zigbee_af_endpoints[i].endpoint = endpoints[i].endpoint;
    sli_zigbee_af_endpoints[i].profileId = endpoints[i].profile_id;
    sli_zigbee_af_endpoints[i].deviceId = endpoints[i].device_id;
    sli_zigbee_af_endpoints[i].deviceVersion = endpoints[i].device_version;
    sli_zigbee_af_endpoints[i].endpointType = endpoint_type_ptr;

    endpoint_type_ptr->clusterCount = endpoints[i].cluster_count;
    endpoint_type_ptr->cluster = cluster_ptr;
    endpoint_type_ptr->endpointSize = 0;

    hal_zigbee_cluster *clusters = endpoints[i].clusters;

    for (int j = 0; j < endpoints[i].cluster_count; j++) {
      cluster_ptr->clusterId = clusters[j].cluster_id;
      cluster_ptr->clusterSize = 0;
      cluster_ptr->attributeCount = clusters[j].attribute_count;
      cluster_ptr->mask =
          clusters[j].is_server ? CLUSTER_MASK_SERVER : CLUSTER_MASK_CLIENT;
      cluster_ptr->attributes = attr_ptr;

      hal_zigbee_attribute *attributes = clusters[j].attributes;

      for (int l = 0; l < clusters[j].attribute_count; l++) {
        attr_ptr->attributeId = attributes[l].attribute_id;
        attr_ptr->attributeType = attributes[l].data_type_id;
        attr_ptr->size = attributes[l].size;
        attr_ptr->mask = ATTRIBUTE_MASK_EXTERNAL_STORAGE;
        if (attributes[l].flag == ATTR_WRITABLE) {
          attr_ptr->mask |= ATTRIBUTE_MASK_WRITABLE;
        }
        attr_ptr++;
      }

      if (clusters[j].cmd_callback) {
        sl_zigbee_subscribe_to_zcl_commands(
            clusters[j].cluster_id, 0xFFFF,
            clusters[j].is_server ? ZCL_DIRECTION_CLIENT_TO_SERVER
                                  : ZCL_DIRECTION_SERVER_TO_CLIENT,
            on_command_callback);
      }
      cluster_ptr++;
    }

    sl_zigbee_af_endpoint_enable_disable(sli_zigbee_af_endpoints[i].endpoint,
                                         true);
    endpoint_type_ptr++;
  }
}

void hal_zigbee_notify_attribute_changed(uint8_t endpoint, uint8_t cluster_id,
                                         uint16_t attribute_id) {
  hal_zigbee_cluster *cluster = find_hal_cluster(endpoint, cluster_id);
  hal_zigbee_attribute *attr =
      find_hal_attr(endpoint, cluster_id, attribute_id);
  if (attr == NULL) {
    return;
  }
  sl_zigbee_af_reporting_attribute_change_cb(
      endpoint, cluster_id, attribute_id,
      cluster->is_server ? CLUSTER_MASK_SERVER : CLUSTER_MASK_CLIENT, 0,
      attr->data_type_id, attr->value);
}

static hal_attribute_change_callback_t attribute_change_callback = NULL;

void hal_zigbee_register_on_attribute_change_callback(
    hal_attribute_change_callback_t callback) {
  attribute_change_callback = callback;
}

sl_zigbee_af_status_t sl_zigbee_af_external_attribute_read_cb(
    uint8_t endpoint, sl_zigbee_af_cluster_id_t clusterId,
    sl_zigbee_af_attribute_metadata_t *attributeMetadata,
    uint16_t manufacturerCode, uint8_t *buffer, uint16_t maxReadLength) {

  hal_zigbee_attribute *attr =
      find_hal_attr(endpoint, clusterId, attributeMetadata->attributeId);
  if (attr == NULL) {
    return SL_ZIGBEE_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
  }
  if (maxReadLength < attr->size) {
    return SL_ZIGBEE_ZCL_STATUS_INSUFFICIENT_SPACE;
  }

  memmove(buffer, attr->value, attr->size);

  return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
}

sl_zigbee_af_status_t sl_zigbee_af_external_attribute_write_cb(
    uint8_t endpoint, sl_zigbee_af_cluster_id_t clusterId,
    sl_zigbee_af_attribute_metadata_t *attributeMetadata,
    uint16_t manufacturerCode, uint8_t *buffer) {
  hal_zigbee_attribute *attr =
      find_hal_attr(endpoint, clusterId, attributeMetadata->attributeId);
  if (attr == NULL) {
    return SL_ZIGBEE_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
  }
  memmove(attr->value, buffer, attr->size);

  if (attribute_change_callback != NULL) {
    attribute_change_callback(endpoint, clusterId, attr->attribute_id);
  }

  return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
}

static bool network_steering_in_progress = false;

hal_zigbee_network_status_t hal_zigbee_get_network_status() {
  sl_zigbee_network_status_t ns = sl_zigbee_af_network_state();
  if (ns == SL_ZIGBEE_JOINED_NETWORK) {
    return HAL_ZIGBEE_NETWORK_JOINED;
  } else if (ns == SL_ZIGBEE_JOINING_NETWORK || network_steering_in_progress) {
    return HAL_ZIGBEE_NETWORK_JOINING;
  } else {
    return HAL_ZIGBEE_NETWORK_NOT_JOINED;
  }
}

static hal_network_status_change_callback_t network_status_change_callback =
    NULL;

void hal_register_on_network_status_change_callback(
    hal_network_status_change_callback_t callback) {
  network_status_change_callback = callback;
}

void sl_zigbee_af_stack_status_callback(sl_status_t status) {
  if (network_status_change_callback != NULL) {
    network_status_change_callback(hal_zigbee_get_network_status());
  }
}

void hal_zigbee_leave_network() {
  sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_OPTION_REJOIN);
}

void hal_zigbee_start_network_steering() {
  if (!network_steering_in_progress) {
    network_steering_in_progress = true;
    sl_zigbee_af_network_steering_start();
  }
}

// Network steering complete callback
void sl_zigbee_af_network_steering_complete_cb(sl_status_t status,
                                               uint8_t totalBeacons,
                                               uint8_t joinAttempts,
                                               uint8_t finalState) {
  network_steering_in_progress = false;

  // Notify application of network status change
  if (network_status_change_callback != NULL) {
    network_status_change_callback(hal_zigbee_get_network_status());
  }
}

static uint8_t make_frame_control(const hal_zigbee_cmd *c) {
  uint8_t fc = 0;
  if (c->cluster_specific)
    fc |= ZCL_CLUSTER_SPECIFIC_COMMAND;
  if (c->direction == HAL_ZIGBEE_DIR_SERVER_TO_CLIENT)
    fc |= ZCL_FRAME_CONTROL_SERVER_TO_CLIENT;
  if (c->disable_default_rsp)
    fc |= ZCL_DISABLE_DEFAULT_RESPONSE_MASK;
  if (c->manufacturer_code)
    fc |= ZCL_MANUFACTURER_SPECIFIC_MASK;
  return fc;
}

static void fill_cmd(const hal_zigbee_cmd *c) {
  sl_zigbee_set_manufacturer_code(c->manufacturer_code);
  uint8_t fc = make_frame_control(c);

  if (c->payload_len == 0 || c->payload == NULL) {
    sl_zigbee_af_fill_external_buffer(fc, c->cluster_id, c->command_id, "");
  } else {
    sl_zigbee_af_fill_external_buffer(fc, c->cluster_id, c->command_id, "b",
                                      c->payload, c->payload_len);
  }
}

hal_zigbee_status_t hal_zigbee_send_cmd_to_bindings(const hal_zigbee_cmd *cmd) {
  if (!cmd)
    return HAL_ZIGBEE_ERR_BAD_ARG;
  if (sl_zigbee_af_network_state() != SL_ZIGBEE_JOINED_NETWORK)
    return HAL_ZIGBEE_ERR_NOT_JOINED;

  fill_cmd(cmd);

  sl_zigbee_af_set_command_endpoints(cmd->endpoint, cmd->endpoint);
  sl_status_t st = sl_zigbee_af_send_command_unicast_to_bindings();
  return (st == SL_STATUS_OK) ? HAL_ZIGBEE_OK : HAL_ZIGBEE_ERR_SEND_FAILED;
}

hal_zigbee_status_t
hal_zigbee_send_report_attr(uint8_t endpoint, uint16_t cluster_id,
                            uint16_t attr_id, uint8_t zcl_type_id,
                            const void *value, uint8_t value_len) {
  if (sl_zigbee_af_network_state() != SL_ZIGBEE_JOINED_NETWORK)
    return HAL_ZIGBEE_ERR_NOT_JOINED;

  uint8_t buf[2 + 1 + 8]; /* attrId(2) + type(1) + value */
  if (value_len > 8)
    return HAL_ZIGBEE_ERR_BAD_ARG;

  buf[0] = (uint8_t)(attr_id & 0xFF);
  buf[1] = (uint8_t)(attr_id >> 8);
  buf[2] = zcl_type_id;
  if (value_len)
    memmove(&buf[3], value, value_len);

  sl_status_t st =
      sl_zigbee_af_fill_command_global_server_to_client_report_attributes(
          cluster_id, buf, 3 + value_len);
  if (st != SL_STATUS_OK)
    return HAL_ZIGBEE_ERR_SEND_FAILED;

  sl_zigbee_af_set_command_endpoints(endpoint, endpoint);
  st = sl_zigbee_af_send_command_unicast_to_bindings();
  return (st == SL_STATUS_OK) ? HAL_ZIGBEE_OK : HAL_ZIGBEE_ERR_SEND_FAILED;
}

hal_zigbee_status_t hal_zigbee_send_announce(void) {
  if (sl_zigbee_send_device_announcement() != SL_STATUS_OK) {
    return HAL_ZIGBEE_ERR_SEND_FAILED;
  }
  return HAL_ZIGBEE_OK;
}

void hal_zigbee_init_ota() {}