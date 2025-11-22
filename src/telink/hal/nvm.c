#include "hal/nvm.h"
#pragma pack(push, 1)
#include "tl_common.h"
#pragma pack(pop)
#include <stdint.h>
#include <string.h>

static hal_nvm_status_t telink_to_hal_status(nv_sts_t telink_status) {
  switch (telink_status) {
  case NV_SUCC:
    return HAL_NVM_SUCCESS;
  case NV_ITEM_NOT_FOUND:
    return HAL_NVM_NOT_FOUND;
  default:
    return HAL_NVM_ERROR;
  }
}

hal_nvm_status_t hal_nvm_write(uint8_t item_id, uint16_t size, uint8_t *data) {
  printf("Writing to NV item %d, size %d\r\n", item_id, size);
  if (data == NULL) {
    return HAL_NVM_ERROR;
  }

  nv_sts_t status = nv_flashWriteNew(1, NV_MODULE_APP, item_id, size, data);
  printf("Write status: %d\r\n", status);
  return telink_to_hal_status(status);
}

hal_nvm_status_t hal_nvm_read(uint8_t item_id, uint16_t size, uint8_t *data) {
  printf("Reading from NV item %d, size %d\r\n", item_id, size);
  if (data == NULL) {
    return HAL_NVM_ERROR;
  }

  nv_sts_t status = nv_flashReadNew(1, NV_MODULE_APP, item_id, size, data);
  return telink_to_hal_status(status);
}

hal_nvm_status_t hal_nvm_delete(uint8_t item_id) {
  // First, get the size of the item to delete
  uint16_t item_size = 0;
  nv_sts_t status =
      nv_flashSingleItemSizeGet(NV_MODULE_APP, item_id, &item_size);

  if (status != NV_SUCC) {
    return telink_to_hal_status(status);
  }

  // Now delete the item
  status = nv_flashSingleItemRemove(NV_MODULE_APP, item_id, item_size);
  return telink_to_hal_status(status);
}

hal_nvm_status_t hal_nvm_clear_all() {
  nv_sts_t status = nv_resetModule(NV_MODULE_APP);
  return telink_to_hal_status(status);
}