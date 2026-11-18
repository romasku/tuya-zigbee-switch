#include "hal/zigbee.h"

#include "app/framework/include/af.h"
#include "app/framework/plugin/ota-client/ota-client.h"
#include "network-steering.h"
#include <stddef.h>
#include <string.h>

#define MAX_CLUSTERS    80
#define MAX_ATTRS       192

sl_zigbee_af_endpoint_type_t      endpoint_type_buffer[ZCL_FIXED_ENDPOINT_COUNT];
sl_zigbee_af_cluster_t            clusters_buffer[MAX_CLUSTERS];
sl_zigbee_af_attribute_metadata_t attributes_buffer[MAX_ATTRS];

hal_zigbee_endpoint *hal_endpoints;
uint8_t hal_endpoints_cnt;
static hal_zcl_activity_callback_t zcl_activity_callback = NULL;

static sl_zigbee_af_endpoint_type_t *     silabs_endpoint_type_ptr;
static sl_zigbee_af_cluster_t *           silabs_cluster_ptr;
static sl_zigbee_af_attribute_metadata_t *silabs_attr_ptr;
static uint8_t                            silabs_fixed_endpoint_index;

static void notify_zcl_activity(void) {
    if (zcl_activity_callback != NULL) {
        zcl_activity_callback();
    }
}

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

    uint8_t *payload     = NULL;
    uint16_t payload_len = 0;
    if (cmd->bufLen > cmd->payloadStartIndex) {
        payload     = cmd->buffer + cmd->payloadStartIndex;
        payload_len = cmd->bufLen - cmd->payloadStartIndex;
    }

    hal_zigbee_cmd_result_t res = hal_cluster->cmd_callback(
        cmd->apsFrame->destinationEndpoint, cmd->apsFrame->clusterId,
        cmd->commandId, payload, payload_len);
    if (res == HAL_ZIGBEE_CMD_SKIPPED) {
        return SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND;
    }
    sl_zigbee_af_status_t status = SL_ZIGBEE_ZCL_STATUS_SUCCESS;
    if (res == HAL_ZIGBEE_INVALID_VALUE) {
        status = SL_ZIGBEE_ZCL_STATUS_INVALID_VALUE;
    } else if (res == HAL_ZIGBEE_MALFORMED_COMMAND) {
        status = SL_ZIGBEE_ZCL_STATUS_MALFORMED_COMMAND;
    } else if (res == HAL_ZIGBEE_ACTION_DENIED) {
        status = SL_ZIGBEE_ZCL_STATUS_ACTION_DENIED;
    }
    sl_zigbee_af_send_immediate_default_response(status);
    return status;
}

bool sl_zigbee_af_pre_command_received_cb(sl_zigbee_af_cluster_command_t *cmd) {
    (void)cmd;
    notify_zcl_activity();
    return false;
}

static uint8_t silabs_register_hal_endpoint(hal_zigbee_endpoint *endpoint) {
    if (silabs_fixed_endpoint_index >= ZCL_FIXED_ENDPOINT_COUNT) {
        return 0;
    }
    uint8_t attr_n = 0;
    for (uint8_t i = 0; i < endpoint->cluster_count; i++) {
        attr_n = (uint8_t)(attr_n + endpoint->clusters[i].attribute_count);
    }
    if (silabs_endpoint_type_ptr >= endpoint_type_buffer + ZCL_FIXED_ENDPOINT_COUNT ||
        silabs_cluster_ptr + endpoint->cluster_count >
            clusters_buffer + MAX_CLUSTERS ||
        silabs_attr_ptr + attr_n > attributes_buffer + MAX_ATTRS) {
        return 0;
    }

    uint8_t index = silabs_fixed_endpoint_index;
    sli_zigbee_af_endpoints[index].endpoint      = endpoint->endpoint;
    sli_zigbee_af_endpoints[index].profileId     = endpoint->profile_id;
    sli_zigbee_af_endpoints[index].deviceId      = endpoint->device_id;
    sli_zigbee_af_endpoints[index].deviceVersion = endpoint->device_version;
    sli_zigbee_af_endpoints[index].endpointType  = silabs_endpoint_type_ptr;

    silabs_endpoint_type_ptr->clusterCount = endpoint->cluster_count;
    silabs_endpoint_type_ptr->cluster      = silabs_cluster_ptr;
    silabs_endpoint_type_ptr->endpointSize = 0;

    for (uint8_t j = 0; j < endpoint->cluster_count; j++) {
        hal_zigbee_cluster *cluster = &endpoint->clusters[j];
        silabs_cluster_ptr->clusterId      = cluster->cluster_id;
        silabs_cluster_ptr->clusterSize    = 0;
        silabs_cluster_ptr->attributeCount = cluster->attribute_count;
        silabs_cluster_ptr->mask           =
            cluster->is_server ? CLUSTER_MASK_SERVER : CLUSTER_MASK_CLIENT;
        silabs_cluster_ptr->attributes = silabs_attr_ptr;

        for (uint8_t l = 0; l < cluster->attribute_count; l++) {
            silabs_attr_ptr->attributeId   = cluster->attributes[l].attribute_id;
            silabs_attr_ptr->attributeType = cluster->attributes[l].data_type_id;
            silabs_attr_ptr->size          = cluster->attributes[l].size;
            silabs_attr_ptr->mask          = ATTRIBUTE_MASK_EXTERNAL_STORAGE;
            if (cluster->attributes[l].flag == ATTR_WRITABLE) {
                silabs_attr_ptr->mask |= ATTRIBUTE_MASK_WRITABLE;
            }
            silabs_attr_ptr++;
        }

        if (cluster->cmd_callback) {
            sl_zigbee_subscribe_to_zcl_commands(
                cluster->cluster_id, 0xFFFF,
                cluster->is_server ? ZCL_DIRECTION_CLIENT_TO_SERVER
                                   : ZCL_DIRECTION_SERVER_TO_CLIENT,
                on_command_callback);
        }
        silabs_cluster_ptr++;
    }

    sl_zigbee_af_endpoint_enable_disable(sli_zigbee_af_endpoints[index].endpoint,
                                         true);
    silabs_endpoint_type_ptr++;
    silabs_fixed_endpoint_index++;
    return 1;
}

void hal_zigbee_init(hal_zigbee_endpoint *endpoints, uint8_t endpoints_cnt) {
    hal_endpoints     = endpoints;
    hal_endpoints_cnt = endpoints_cnt;

    for (int i = 0; i < ZCL_FIXED_ENDPOINT_COUNT; i++) {
        sl_zigbee_af_endpoint_enable_disable(sli_zigbee_af_endpoints[i].endpoint,
                                             false);
    }

    // Avoid settings more endpoints then device supports
    endpoints_cnt = endpoints_cnt <= ZCL_FIXED_ENDPOINT_COUNT
                      ? endpoints_cnt
                      : ZCL_FIXED_ENDPOINT_COUNT;
    hal_endpoints_cnt = endpoints_cnt;

    silabs_endpoint_type_ptr    = endpoint_type_buffer;
    silabs_cluster_ptr          = clusters_buffer;
    silabs_attr_ptr             = attributes_buffer;
    silabs_fixed_endpoint_index = 0;

    for (int i = 0; i < endpoints_cnt; i++) {
        silabs_register_hal_endpoint(&endpoints[i]);
    }
}

uint8_t hal_zigbee_add_endpoint(hal_zigbee_endpoint *endpoint) {
    if (hal_endpoints == NULL || endpoint == NULL ||
        endpoint != &hal_endpoints[hal_endpoints_cnt] ||
        hal_endpoints_cnt >= ZCL_FIXED_ENDPOINT_COUNT) {
        return 0;
    }
    if (!silabs_register_hal_endpoint(endpoint)) {
        return 0;
    }
    hal_endpoints_cnt++;
    return 1;
}

void hal_zigbee_notify_attribute_changed(uint8_t endpoint, uint16_t cluster_id,
                                         uint16_t attribute_id) {
    hal_zigbee_cluster *  cluster = find_hal_cluster(endpoint, cluster_id);
    hal_zigbee_attribute *attr    =
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

void hal_zigbee_register_on_zcl_activity_callback(
    hal_zcl_activity_callback_t callback) {
    zcl_activity_callback = callback;
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

static void notify_network_status_change(void) {
    if (network_status_change_callback != NULL) {
        network_status_change_callback(hal_zigbee_get_network_status());
    }
}

void hal_register_on_network_status_change_callback(
    hal_network_status_change_callback_t callback) {
    network_status_change_callback = callback;
}

void sl_zigbee_af_stack_status_cb(sl_status_t status) {
    (void)status;
    notify_network_status_change();
}

void hal_zigbee_leave_network() {
    sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_NO_OPTION);
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
    (void)status;
    (void)totalBeacons;
    (void)joinAttempts;
    (void)finalState;
    network_steering_in_progress = false;

    notify_network_status_change();
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
    sl_status_t st_unicast   = sl_zigbee_af_send_command_unicast_to_bindings();
    sl_status_t st_multicast = sl_zigbee_af_send_command_multicast_to_bindings();

    return (st_unicast == SL_STATUS_OK || st_multicast == SL_STATUS_OK)
           ? HAL_ZIGBEE_OK : HAL_ZIGBEE_ERR_SEND_FAILED;
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

static bool hal_zigbee_poll_rate_supported(void) {
    sl_zigbee_node_type_t node_type = SL_ZIGBEE_UNKNOWN_DEVICE;

    if (sl_zigbee_af_get_node_type(&node_type) != SL_STATUS_OK) {
        return false;
    }

    return node_type == SL_ZIGBEE_END_DEVICE ||
           node_type == SL_ZIGBEE_SLEEPY_END_DEVICE ||
           node_type == SL_ZIGBEE_S2S_INITIATOR_DEVICE ||
           node_type == SL_ZIGBEE_S2S_TARGET_DEVICE;
}

void hal_zigbee_set_poll_rate_ms(uint32_t poll_rate_ms) {
    if (!hal_zigbee_poll_rate_supported()) {
        return;
    }
    // Only set the long poll interval, keep short poll managed by
    // SDK itself for Silabs
    sl_zigbee_af_set_long_poll_interval_ms_cb(poll_rate_ms);
}

uint32_t hal_zigbee_get_poll_rate_ms(void) {
    if (!hal_zigbee_poll_rate_supported()) {
        return 0;
    }

    return sl_zigbee_af_get_long_poll_interval_ms_cb();
}

void hal_zigbee_init_ota() {
}
