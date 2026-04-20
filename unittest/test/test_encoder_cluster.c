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

void _action_casues_sending_zigbee_command(ev_encoder_callback_t action, uint8_t cluster_id, uint8_t command_id, uint8_t payload[], int payload_length) 
{
  // Always report the zigbee status as connected
  hal_zigbee_get_network_status_ExpectAndReturn(HAL_ZIGBEE_NETWORK_JOINED);

  // Capture zigbee commands sent
  hal_zigbee_send_cmd_to_bindings_Stub(captured_send_cmd_to_bindings);

  // Trigger the  event
  action(mock_encoder.callback_param);

  // Check one command was sent 
  TEST_ASSERT_EQUAL_MESSAGE(1, send_cmd_call_count, "Unexpected number of commands sent");

  // Check command was step brightnes up
  TEST_ASSERT_EQUAL(mock_endpoint.endpoint, captured_commands[0].endpoint);  
  TEST_ASSERT_EQUAL(ZCL_HA_PROFILE, captured_commands[0].profile_id);  
  TEST_ASSERT_EQUAL(cluster_id, captured_commands[0].cluster_id);  
  TEST_ASSERT_EQUAL(command_id, captured_commands[0].command_id);
  TEST_ASSERT_EQUAL(HAL_ZIGBEE_DIR_CLIENT_TO_SERVER, captured_commands[0].direction);
 
  if(payload_length != 0) {
    TEST_ASSERT_EQUAL_INT8_ARRAY(payload, captured_commands[0].payload, payload_length);
  }


  TEST_ASSERT_EQUAL(payload_length, captured_commands[0].payload_len);
}

void test_encoder_is_clicked(void)
{
  // Toggle On Off Zigbee commmand should be sent
  _action_casues_sending_zigbee_command(mock_encoder.on_press, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE, NULL, 0);

}

void test_encoder_is_rotated_cw(void)
{
  uint8_t payload[4];
    payload[0] = ZCL_LEVEL_MOVE_UP; // Step Mode 0 up, 1 down
    payload[1] = 13; // Step size  
    // Transistion Time
    payload[2] = 0; 
    payload[3] = 0;

  // Step Brightness Up Zigbee commmand should be sent
  _action_casues_sending_zigbee_command(mock_encoder.on_rotate_cw, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STEP, payload, 4);
}