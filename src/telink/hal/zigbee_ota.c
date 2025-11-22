#pragma pack(push, 1)
#include "tl_common.h"
#include "zcl_include.h"
#include "ota.h"
#pragma pack(pop)

#include "telink_size_t_hack.h"

#include "hal/zigbee_ota.h"
#include "telink_zigbee_hal.h"
#include "version_cfg.h"

// Forward declarations

void ota_process_msg_callback(u8 evt, u8 status);

// ota data structs

ota_preamble_t ota_preamble = {
    .fileVer = FILE_VERSION,
    .imageType = IMAGE_TYPE,
    .manufacturerCode = MANUFACTURER_CODE_TELINK,
};

ota_callBack_t ota_callback = {
    ota_process_msg_callback,
};

void hal_ota_cluster_setup(hal_zigbee_cluster *cluster) {
  if (cluster == NULL) {
    return;
  }
  cluster->cluster_id = ZCL_CLUSTER_OTA;
  cluster->is_server = 0;
  // Attrs are managed by SDK internally
}

void ota_process_msg_callback(u8 evt, u8 status) {
  if (evt == OTA_EVT_COMPLETE) {
    if (status == ZCL_STA_SUCCESS) {
      ota_mcuReboot();
    } else {
      ota_queryStart(OTA_PERIODIC_QUERY_INTERVAL);
    }
  }
}

void hal_zigbee_init_ota() {
  // This registers OTA cluster in ZCL and does all SDK-internal setup
  ota_init(OTA_TYPE_CLIENT, telink_zigbee_hal_zcl_get_descriptors(),
           &ota_preamble, &ota_callback);
}