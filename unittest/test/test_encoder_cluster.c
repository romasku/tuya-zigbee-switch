#include "unity.h"
#include "zigbee/encoder_cluster.h"
#include "zigbee/consts.h"
#include "Mockzigbee.h"

void setUp(void)
{
  // Put a space between tests for readability
  printf("\r\n");
}

void tearDown(void)
{
}

int send_cmd_call_count = 0;
hal_zigbee_cmd *captured_commands[10];
hal_zigbee_status_t captured_send_cmd_to_bindings(const hal_zigbee_cmd *cmd, int cmock_num_calls)
{
  send_cmd_call_count++;
  captured_commands[cmock_num_calls] = cmd;
  return HAL_ZIGBEE_OK;
}


void test_encoder_is_clicked(void)
{
  // Toggle On Off Zigbee commmand should be sent

  hal_zigbee_endpoint mock_endpoint = {};
  mock_endpoint.endpoint = 1;
  mock_endpoint.cluster_count = 0;
  hal_zigbee_cluster endpoint_clusters[10];
  mock_endpoint.clusters = endpoint_clusters;

  encoder_t mock_encoder = {};

  zigbee_encoder_cluster encoder_cluster = {};
  encoder_cluster.encoder = &mock_encoder;

  encoder_cluster_add_to_endpoint(&encoder_cluster, &mock_endpoint);

  hal_zigbee_get_network_status_ExpectAndReturn(HAL_ZIGBEE_NETWORK_JOINED);

  hal_zigbee_send_cmd_to_bindings_Stub(captured_send_cmd_to_bindings);

  mock_encoder.on_press(mock_encoder.callback_param);

  // Check one command was sent 
  TEST_ASSERT_EQUAL(1, send_cmd_call_count);

  // Check command was toggle on/off
  TEST_ASSERT_EQUAL(mock_endpoint.endpoint, captured_commands[0]->endpoint);  
  TEST_ASSERT_EQUAL(ZCL_HA_PROFILE, captured_commands[0]->profile_id);  
  TEST_ASSERT_EQUAL(ZCL_CLUSTER_ON_OFF, captured_commands[0]->cluster_id);  
  TEST_ASSERT_EQUAL(ZCL_CMD_ONOFF_TOGGLE, captured_commands[0]->command_id);
  TEST_ASSERT_EQUAL(HAL_ZIGBEE_DIR_CLIENT_TO_SERVER, captured_commands[0]->direction);
  TEST_ASSERT_NULL(captured_commands[0]->payload);
  TEST_ASSERT_EQUAL(0, captured_commands[0]->payload_len);

}