#include "unity.h"
#include "zigbee/encoder_cluster.h"
#include "zigbee/consts.h"
#include "Mockzigbee.h"

int send_cmd_call_count = 0;
hal_zigbee_cmd captured_commands[10];
hal_zigbee_status_t captured_send_cmd_to_bindings(const hal_zigbee_cmd *cmd, int cmock_num_calls)
{
  send_cmd_call_count++;
  captured_commands[cmock_num_calls] = *cmd;
  return HAL_ZIGBEE_OK;
}

hal_zigbee_endpoint mock_endpoint = {};
hal_zigbee_cluster endpoint_clusters[10];
encoder_t mock_encoder = {};
zigbee_encoder_cluster encoder_cluster = {};

void setUp(void)
{
  // Put a space between tests for readability
  printf("\r\n");

  send_cmd_call_count = 0;

  // Setup endpoint, cluster and encoder 
  mock_endpoint.endpoint = 1;
  hal_zigbee_cluster endpoint_clusters[10];
  mock_endpoint.clusters = endpoint_clusters;
  mock_endpoint.cluster_count = 0;
  encoder_cluster.encoder = &mock_encoder;
  encoder_cluster_add_to_endpoint(&encoder_cluster, &mock_endpoint);
}

void tearDown(void)
{
}

void test_encoder_is_clicked(void)
{
  // Toggle On Off Zigbee commmand should be sent

  // Setup
  // Always report the zigbee status as connected
  hal_zigbee_get_network_status_IgnoreAndReturn(HAL_ZIGBEE_NETWORK_JOINED);
 
  // Capture zigbee commands sent
  hal_zigbee_send_cmd_to_bindings_Stub(captured_send_cmd_to_bindings);

  // Trigger a on press event
  mock_encoder.on_press(mock_encoder.callback_param);

  // Check one command was sent 
  TEST_ASSERT_EQUAL_MESSAGE(1, send_cmd_call_count, "Unexpected number of commands sent");

  // Check command was toggle on/off
  TEST_ASSERT_EQUAL(mock_endpoint.endpoint, captured_commands[0].endpoint);  
  TEST_ASSERT_EQUAL(ZCL_HA_PROFILE, captured_commands[0].profile_id);  
  TEST_ASSERT_EQUAL(ZCL_CLUSTER_ON_OFF, captured_commands[0].cluster_id);  
  TEST_ASSERT_EQUAL(ZCL_CMD_ONOFF_TOGGLE, captured_commands[0].command_id);
  TEST_ASSERT_EQUAL(HAL_ZIGBEE_DIR_CLIENT_TO_SERVER, captured_commands[0].direction);
  TEST_ASSERT_NULL(captured_commands[0].payload);
  TEST_ASSERT_EQUAL(0, captured_commands[0].payload_len);
}

void test_encoder_is_rotated_cw(void)
{
  // Step Brightness Up Zigbee commmand should be sent
  // Always report the zigbee status as connected
  hal_zigbee_get_network_status_ExpectAndReturn(HAL_ZIGBEE_NETWORK_JOINED);

  // Capture zigbee commands sent
  hal_zigbee_send_cmd_to_bindings_Stub(captured_send_cmd_to_bindings);

  // Trigger a rotate clockwise event
  mock_encoder.on_rotate_cw(mock_encoder.callback_param);

  // Check one command was sent 
  TEST_ASSERT_EQUAL_MESSAGE(1, send_cmd_call_count, "Unexpected number of commands sent");

  // Check command was step brightnes up
  TEST_ASSERT_EQUAL(mock_endpoint.endpoint, captured_commands[0].endpoint);  
  TEST_ASSERT_EQUAL(ZCL_HA_PROFILE, captured_commands[0].profile_id);  
  TEST_ASSERT_EQUAL(ZCL_CLUSTER_LEVEL_CONTROL, captured_commands[0].cluster_id);  
  TEST_ASSERT_EQUAL(ZCL_CMD_LEVEL_STEP, captured_commands[0].command_id);
  TEST_ASSERT_EQUAL(HAL_ZIGBEE_DIR_CLIENT_TO_SERVER, captured_commands[0].direction);

  uint8_t payload[4];
    payload[0] = ZCL_LEVEL_MOVE_UP; // Step Mode 0 up, 1 down
    payload[1] = 13; // Step size  
    // Transistion Time
    payload[2] = 0; 
    payload[3] = 0;
  
  //TEST_ASSERT_EQUAL(4, sizeof(captured_commands[0].payload)); // Why is this 8?
  TEST_ASSERT_EQUAL_INT8_ARRAY(payload, captured_commands[0].payload, sizeof(payload));
  TEST_ASSERT_EQUAL(4, captured_commands[0].payload_len);
}