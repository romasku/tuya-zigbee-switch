#include "encoder_cluster.h"
#include "consts.h"

#include "hal/printf_selector.h"
#include "zigbee_commands.h"

void encoder_cluster_on_button_press(zigbee_encoder_cluster *cluster);
void encoder_cluster_on_rotate_cw(zigbee_encoder_cluster *cluster);
void encoder_cluster_on_rotate_ccw(zigbee_encoder_cluster *cluster);
void encoder_cluster_on_rotate_cw_pressed(zigbee_encoder_cluster *cluster);
void encoder_cluster_on_rotate_ccw_pressed(zigbee_encoder_cluster *cluster);

// Not currently used, but will be when we implement attribute writing callback
zigbee_encoder_cluster *encoder_cluster_by_endpoint[10];

void encoder_cluster_add_to_endpoint(zigbee_encoder_cluster *cluster,
                                     hal_zigbee_endpoint *endpoint)
{
  encoder_cluster_by_endpoint[endpoint->endpoint] = cluster;
  
  cluster->endpoint = endpoint->endpoint;

  //////// Cluster Setup ////////
  // Output ON OFF to bind to other devices
  endpoint->clusters[endpoint->cluster_count].cluster_id = ZCL_CLUSTER_ON_OFF;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
  endpoint->clusters[endpoint->cluster_count].attributes = NULL;
  endpoint->clusters[endpoint->cluster_count].is_server = 0;
  endpoint->cluster_count++;

  // Output Level (Brightness) for other devices
  endpoint->clusters[endpoint->cluster_count].cluster_id =
      ZCL_CLUSTER_LEVEL_CONTROL;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
  endpoint->clusters[endpoint->cluster_count].attributes = NULL;
  endpoint->clusters[endpoint->cluster_count].is_server = 0;
  endpoint->cluster_count++;

   // Output lightingColorCtrl for other devices
  endpoint->clusters[endpoint->cluster_count].cluster_id =
      ZCL_CLUSTER_LIGHTING_COLOR_CONTROL;
  endpoint->clusters[endpoint->cluster_count].attribute_count = 0;
  endpoint->clusters[endpoint->cluster_count].attributes = NULL;
  endpoint->clusters[endpoint->cluster_count].is_server = 0;
  endpoint->cluster_count++;
  //////// End of Cluster Setup ////////

  // Link Encoder Callbacks to this clusters actions
  cluster->encoder->on_press =
      (ev_encoder_callback_t)encoder_cluster_on_button_press;
  cluster->encoder->on_rotate_cw = (ev_encoder_callback_t)encoder_cluster_on_rotate_cw;
  cluster->encoder->on_rotate_ccw = (ev_encoder_callback_t)encoder_cluster_on_rotate_ccw;
  cluster->encoder->on_rotate_cw_while_pressed = (ev_encoder_callback_t)encoder_cluster_on_rotate_cw_pressed;
  cluster->encoder->on_rotate_ccw_while_pressed = (ev_encoder_callback_t)encoder_cluster_on_rotate_ccw_pressed;
  cluster->encoder->callback_param = cluster;
}

void build_and_send_toggle_on_off_command(zigbee_encoder_cluster *cluster) 
{
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED)
  {
    return;
  }

  printf("Sending Toggle On Off Command\r\n");

  hal_zigbee_cmd c = build_onoff_cmd(cluster->endpoint, ZCL_CMD_ONOFF_TOGGLE);
  hal_zigbee_send_cmd_to_bindings(&c);
}

void build_and_send_brightness_command(zigbee_encoder_cluster *cluster, uint8_t dir)
{
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED)
  {
    return;
  }

  printf("Sending Level Step Command\r\n");

  hal_zigbee_cmd c = build_level_step_cmd(cluster->endpoint, dir, 13);
  hal_zigbee_send_cmd_to_bindings(&c);
}

void build_and_send_color_temp_command(zigbee_encoder_cluster *cluster, uint8_t dir)
{
  if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED)
  {
    return;
  }

  printf("Sending Color Temp Step Command\r\n");

  hal_zigbee_cmd c = build_color_temp_step_cmd(cluster->endpoint, dir, 13);
  hal_zigbee_send_cmd_to_bindings(&c);
}

void encoder_cluster_on_button_press(zigbee_encoder_cluster *cluster)
{
  printf("Encoder Cluster button pressed cb triggered\r\n");
  build_and_send_toggle_on_off_command(cluster);
}

void encoder_cluster_on_rotate_cw(zigbee_encoder_cluster *cluster)
{
  printf("Encoder Cluster rotate cw cb triggered\r\n");
  build_and_send_brightness_command(cluster, ZCL_LEVEL_MOVE_UP);
}

void encoder_cluster_on_rotate_ccw(zigbee_encoder_cluster *cluster)
{
  printf("Encoder Cluster rotate ccw cb triggered\r\n");
  build_and_send_brightness_command(cluster, ZCL_LEVEL_MOVE_DOWN);
}

void encoder_cluster_on_rotate_cw_pressed(zigbee_encoder_cluster *cluster)
{
  printf("Encoder Cluster rotate cw pressed cb triggered\r\n");
  build_and_send_color_temp_command(cluster, 0x01);
}

void encoder_cluster_on_rotate_ccw_pressed(zigbee_encoder_cluster *cluster)
{
  printf("Encoder Cluster rotate ccw pressed cb triggered\r\n");
  build_and_send_color_temp_command(cluster, 0x03);
}