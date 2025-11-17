#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "nvm_items.h"

#define UNKNOWN_VERSION 0

uint16_t read_version_in_nv() {
  uint16_t version;

  hal_nvm_status_t res = hal_nvm_read(NV_ITEM_CURRENT_VERSION_IN_NV,
                                      sizeof(version), (uint8_t *)&version);
  if (res == HAL_NVM_SUCCESS) {
    printf("read version form new location\r\n");
    return version;
  }

  return UNKNOWN_VERSION;
}

void write_version_to_nv(uint16_t version) {

  hal_nvm_status_t res = hal_nvm_write(NV_ITEM_CURRENT_VERSION_IN_NV,
                                       sizeof(version), (uint8_t *)&version);
  if (res != HAL_NVM_SUCCESS) {
    printf("Failed to write lastSeenVersion to NV, st: %d\r\n", res);
  }
}

void handle_version_changes() {
  uint16_t oldVersion = read_version_in_nv();
  uint16_t currentVersion = NVM_MIGRATIONS_VERSION;

  printf("Old version: %d\r\n", oldVersion);
  printf("Current version: %d\r\n", currentVersion);

  if (oldVersion == currentVersion) {
    // Same version, nothing to do
    return;
  }

  if (oldVersion == UNKNOWN_VERSION) {
    // Either old device or it first boot after re-flash, just store version
    write_version_to_nv(currentVersion);
    return;
  }

  // Handle migrations here
  // Example:
  // if (oldVersion < XX) {
  //   migrate_to_vXX();
  // }
}