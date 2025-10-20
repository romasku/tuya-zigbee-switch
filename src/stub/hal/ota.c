#include "hal/zigbee.h"
#include "hal/zigbee_ota.h"

// ZCL data type constants for stub build
#define ZCL_IEEE_ADDRESS_ATTRIBUTE_TYPE 0xF0
#define ZCL_INT32U_ATTRIBUTE_TYPE 0x23
#define ZCL_ENUM8_ATTRIBUTE_TYPE 0x30
#define ZCL_OTA_BOOTLOAD_CLUSTER_ID 0x19

static struct OtaData {
  uint64_t upgrade_server_id;
  uint32_t offset;
  uint8_t status;
} ota_data;

hal_zigbee_attribute attrs[] = {
    {.attribute_id = 0x0000,
     .data_type_id = ZCL_IEEE_ADDRESS_ATTRIBUTE_TYPE,
     .size = 8,
     .value = (uint8_t *)&ota_data.upgrade_server_id,
     .flag = ATTR_WRITABLE},
    {.attribute_id = 0x0001,
     .data_type_id = ZCL_INT32U_ATTRIBUTE_TYPE,
     .size = 4,
     .value = (uint8_t *)&ota_data.offset,
     .flag = ATTR_WRITABLE},
    {.attribute_id = 0x0006,
     .data_type_id = ZCL_ENUM8_ATTRIBUTE_TYPE,
     .size = 1,
     .value = (uint8_t *)&ota_data.status,
     .flag = ATTR_WRITABLE},
};

void hal_ota_cluster_setup(hal_zigbee_cluster *cluster) {
  if (cluster == NULL) {
    return;
  }
  cluster->cluster_id = ZCL_OTA_BOOTLOAD_CLUSTER_ID;
  cluster->is_server = false;
  cluster->attribute_count = 3;
  cluster->attributes = attrs;
  cluster->cmd_callback = NULL;
}

void hal_zigbee_init_ota() {
  // Stub implementation - no initialization needed
}