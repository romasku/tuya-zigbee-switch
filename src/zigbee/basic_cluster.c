#include "basic_cluster.h"
#include "base_components/network_indicator.h"
#include "build_date.h"
#include "cluster_common.h"
#include "consts.h"
#include "device_config/config_nv.h"
#include "device_config/nvm_items.h"
#include "hal/nvm.h"
#include "hal/system.h"
#include "hal/tasks.h"
#include <stddef.h>

const uint8_t zclVersion = 0x03;
const uint8_t appVersion = 0x03;
const uint8_t stackVersion = 0x02;
const uint8_t hwVersion = 0x00;

const uint8_t powerSource = 0x01; // POWER_SOURCE_MAINS_1_PHASE

const uint16_t cluster_revision = 0x01;
DEF_STR(STRINGIFY_VALUE(VERSION_STR), swBuildId);
extern network_indicator_t network_indicator;

void basic_cluster_store_attrs_to_nv();
void basic_cluster_load_attrs_from_nv();

void basic_cluster_reset_handler(void *arg) { hal_system_reset(); }

static hal_task_t reset_task = {.handler = basic_cluster_reset_handler};

void basic_cluster_callback_attr_write_trampoline(uint16_t attribute_id) {
  basic_cluster_store_attrs_to_nv();
  if (attribute_id == ZCL_ATTR_BASIC_DEVICE_CONFIG) {
    device_config_str.data[device_config_str.size] =
        0; // NULL terminate the string
    device_config_write_to_nv();
    hal_tasks_init(&reset_task);
    hal_tasks_schedule(&reset_task, 300);
  }
  if (attribute_id == ZCL_ATTR_BASIC_STATUS_LED_STATE) {
    network_indicator_from_manual_state(&network_indicator);
  }
}

void basic_cluster_add_to_endpoint(zigbee_basic_cluster *cluster,
                                   hal_zigbee_endpoint *endpoint) {

  // Initialize build date buffer
  zb_build_date_init(ZB_BUILD_DATE_YYYYMMDD);

  // Fill Attrs

  SETUP_ATTR(0, ZCL_ATTR_BASIC_ZCL_VER, ZCL_DATA_TYPE_UINT8, ATTR_READONLY,
             zclVersion);

  SETUP_ATTR(1, ZCL_ATTR_BASIC_APP_VER, ZCL_DATA_TYPE_UINT8, ATTR_READONLY,
             appVersion);
  SETUP_ATTR(2, ZCL_ATTR_BASIC_STACK_VER, ZCL_DATA_TYPE_UINT8, ATTR_READONLY,
             stackVersion);
  SETUP_ATTR(3, ZCL_ATTR_BASIC_HW_VER, ZCL_DATA_TYPE_UINT8, ATTR_READONLY,
             hwVersion);
  SETUP_ATTR(4, ZCL_ATTR_BASIC_MFR_NAME, ZCL_DATA_TYPE_CHAR_STR, ATTR_READONLY,
             cluster->manuName);
  SETUP_ATTR(5, ZCL_ATTR_BASIC_MODEL_ID, ZCL_DATA_TYPE_CHAR_STR, ATTR_READONLY,
             cluster->modelId);
  SETUP_ATTR(6, ZCL_ATTR_BASIC_POWER_SOURCE, ZCL_DATA_TYPE_ENUM8, ATTR_READONLY,
             powerSource);
  SETUP_ATTR(7, ZCL_ATTR_BASIC_DEV_ENABLED, ZCL_DATA_TYPE_BOOLEAN,
             ATTR_WRITABLE, cluster->deviceEnable);
  SETUP_ATTR(8, ZCL_ATTR_BASIC_SW_BUILD_ID, ZCL_DATA_TYPE_CHAR_STR,
             ATTR_READONLY, swBuildId);
  SETUP_ATTR(9, ZCL_ATTR_BASIC_DATE_CODE, ZCL_DATA_TYPE_CHAR_STR, ATTR_READONLY,
             ZB_BUILD_DATE_YYYYMMDD);
  SETUP_ATTR(10, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
             ATTR_READONLY, cluster_revision);
  SETUP_ATTR(11, ZCL_ATTR_BASIC_DEVICE_CONFIG, ZCL_DATA_TYPE_LONG_CHAR_STR,
             ATTR_WRITABLE, device_config_str);
  if (network_indicator.has_dedicated_led) {
    SETUP_ATTR(12, ZCL_ATTR_BASIC_STATUS_LED_STATE, ZCL_DATA_TYPE_BOOLEAN,
               ATTR_WRITABLE, network_indicator.manual_state_when_connected);
  }

  endpoint->clusters[endpoint->cluster_count].cluster_id = ZCL_CLUSTER_BASIC;
  endpoint->clusters[endpoint->cluster_count].attribute_count =
      network_indicator.has_dedicated_led ? 13 : 12;
  endpoint->clusters[endpoint->cluster_count].attributes = cluster->attr_infos;
  endpoint->clusters[endpoint->cluster_count].is_server = 1;
  endpoint->cluster_count++;

  basic_cluster_load_attrs_from_nv();
  if (hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED &&
      network_indicator.has_dedicated_led) {
    network_indicator_from_manual_state(&network_indicator);
  }
}

typedef struct {
  uint8_t network_led_on;
} zigbee_basic_cluster_config;

static zigbee_basic_cluster_config nv_config_buffer;

void basic_cluster_store_attrs_to_nv() {
  nv_config_buffer.network_led_on =
      network_indicator.manual_state_when_connected;

  hal_nvm_write(NV_ITEM_BASIC_CLUSTER_DATA, sizeof(zigbee_basic_cluster_config),
                (uint8_t *)&nv_config_buffer);
}

void basic_cluster_load_attrs_from_nv() {
  hal_nvm_status_t st = hal_nvm_read(NV_ITEM_BASIC_CLUSTER_DATA,
                                     sizeof(zigbee_basic_cluster_config),
                                     (uint8_t *)&nv_config_buffer);

  if (st != HAL_NVM_SUCCESS) {
    return;
  }
  network_indicator.manual_state_when_connected =
      nv_config_buffer.network_led_on;
}
