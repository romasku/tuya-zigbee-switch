#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "nvm_items.h"
#include "zigbee/consts.h"
#include "zigbee/switch_cluster.h"

#ifdef HAL_SILABS
#include "silabs_config.h"
#endif

#define UNKNOWN_VERSION    0

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

void migrate_to_v2() {
    extern uint8_t switch_clusters_cnt;

    for (int i = 0; i < switch_clusters_cnt; i++) {
        typedef struct __attribute__((packed)) {
            uint8_t mode;
            uint8_t action;
            uint8_t relay_mode;
            uint8_t relay_index;
            uint16_t button_long_press_duration;
            uint8_t level_move_rate;
            uint8_t binded_mode;
        } old_zigbee_switch_cluster_config;
        
        old_zigbee_switch_cluster_config old_config;
        hal_nvm_status_t st = hal_nvm_read(
            NV_ITEM_SWITCH_CLUSTER_DATA(i),
            sizeof(old_zigbee_switch_cluster_config), (uint8_t *)&old_config);

        if (st == HAL_NVM_SUCCESS) {
            zigbee_switch_cluster_config new_config;
            new_config.mode = old_config.mode;
            new_config.action = old_config.action;
            new_config.relay_mode = old_config.relay_mode;
            new_config.relay_index = old_config.relay_index;
            new_config.button_long_press_duration = old_config.button_long_press_duration;
            new_config.level_move_rate = old_config.level_move_rate;
            new_config.binded_mode = old_config.binded_mode;
            new_config.level_move_mode = ZCL_LEVEL_MOVE_MODE_ALTERNATE; // Default value
            
            // Write the updated configuration
            st = hal_nvm_write(NV_ITEM_SWITCH_CLUSTER_DATA(i),
                               sizeof(zigbee_switch_cluster_config),
                               (uint8_t *)&new_config);
            
            if (st != HAL_NVM_SUCCESS) {
                printf("Failed to migrate switch cluster %d config\r\n", i);
            } else {
                printf("Migrated switch cluster %d config\r\n", i);
            }
        }
    }
}

void handle_version_changes() {
    uint16_t oldVersion     = read_version_in_nv();
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
    if (oldVersion < 2) {
        migrate_to_v2();
    }
    // After all migrations, update the version in NV
    write_version_to_nv(currentVersion);
}
