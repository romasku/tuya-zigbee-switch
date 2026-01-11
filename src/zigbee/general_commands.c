#include "basic_cluster.h"
#include "consts.h"
#include "hal/printf_selector.h"
#include "relay_cluster.h"
#include "switch_cluster.h"

static void zigbee_on_attr_change(uint8_t endpoint, uint16_t cluster_id,
                                  uint16_t attribute_id) {
  printf("Attribute changed, ep: %d, cluster: %d, attr: %d\r\n", endpoint,
         cluster_id, attribute_id);
  if (cluster_id == ZCL_CLUSTER_BASIC) {
    basic_cluster_callback_attr_write_trampoline(attribute_id);
  } else if (cluster_id == ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG) {
    switch_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
  } else if (cluster_id == ZCL_CLUSTER_ON_OFF) {
    relay_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
  }
}

void init_global_attr_write_callback() {
  hal_zigbee_register_on_attribute_change_callback(zigbee_on_attr_change);
}
