#include "hal/nvm.h"
#include "nvm3.h"
#include "nvm3_default.h"

// Convert NVM3 status codes to HAL status codes
static hal_nvm_status_t nvm3_to_hal_status(Ecode_t nvm3_status) {
  switch (nvm3_status) {
  case ECODE_NVM3_OK:
    return HAL_NVM_SUCCESS;
  case ECODE_NVM3_ERR_KEY_NOT_FOUND:
    return HAL_NVM_NOT_FOUND;
  default:
    return HAL_NVM_ERROR;
  }
}

hal_nvm_status_t hal_nvm_read(uint8_t item_id, uint16_t size, uint8_t *data) {
  if (data == NULL) {
    return HAL_NVM_ERROR;
  }

  Ecode_t status = nvm3_readData(nvm3_defaultHandle, item_id, data, size);
  return nvm3_to_hal_status(status);
}

hal_nvm_status_t hal_nvm_write(uint8_t item_id, uint16_t size, uint8_t *data) {
  if (data == NULL) {
    return HAL_NVM_ERROR;
  }

  Ecode_t status = nvm3_writeData(nvm3_defaultHandle, item_id, data, size);
  return nvm3_to_hal_status(status);
}

hal_nvm_status_t hal_nvm_delete(uint8_t item_id) {
  Ecode_t status = nvm3_deleteObject(nvm3_defaultHandle, item_id);
  return nvm3_to_hal_status(status);
}

hal_nvm_status_t hal_nvm_clear_all() {
  nvm3_ObjectKey_t buffer[10];
  size_t keys_count = 0;
  nvm3_ObjectKey_t min_key = 0;

  while ((keys_count = nvm3_enumObjects(nvm3_defaultHandle, buffer, 10, min_key,
                                        0x0FF // We allow only up to 256 keys
                                        )) != 0) {
    for (nvm3_ObjectKey_t *key = buffer; key < buffer + keys_count; key++) {
      Ecode_t status = nvm3_deleteObject(nvm3_defaultHandle, *key);
      if (status != ECODE_NVM3_OK && status != ECODE_NVM3_ERR_KEY_NOT_FOUND) {
        // Continue deleting other keys even if one fails, but return error at
        // end
        return nvm3_to_hal_status(status);
      }
      min_key = min_key <= *key ? *key + 1 : min_key;
    }
  }
  return HAL_NVM_SUCCESS;
}
