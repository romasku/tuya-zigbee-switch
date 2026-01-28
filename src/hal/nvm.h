#ifndef _HAL_NVM_H_
#define _HAL_NVM_H_

#include <stdint.h>

#define HAL_NVM_SUCCESS      0
#define HAL_NVM_NOT_FOUND    1
#define HAL_NVM_ERROR        2

/** NVM operation result (HAL_NVM_SUCCESS = 0 on success) */
typedef uint32_t hal_nvm_status_t;

/**
 * Store data in non-volatile memory for persistence across reboots
 * @param item_id Unique identifier for the data item
 * @param size Number of bytes to write
 * @param data Buffer containing data to store
 * @return HAL_NVM_SUCCESS on success, error code otherwise
 */
hal_nvm_status_t hal_nvm_write(uint8_t item_id, uint16_t size, uint8_t *data);

/**
 * Retrieve previously stored data from non-volatile memory
 * @param item_id Unique identifier for the data item
 * @param size Number of bytes to read
 * @param data Buffer to receive the data
 * @return HAL_NVM_SUCCESS on success, error code if item not found
 */
hal_nvm_status_t hal_nvm_read(uint8_t item_id, uint16_t size, uint8_t *data);

/**
 * Remove stored data item from non-volatile memory
 * @param item_id Unique identifier for the data item to delete
 * @return HAL_NVM_SUCCESS on success, error code otherwise
 */
hal_nvm_status_t hal_nvm_delete(uint8_t item_id);

/**
 * Erase all stored data items (factory reset functionality)
 * @return HAL_NVM_SUCCESS on success, error code otherwise
 */
hal_nvm_status_t hal_nvm_clear_all();

#endif
