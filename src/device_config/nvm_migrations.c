#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "nvm_items.h"

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

/* MAX_SWITCHES and MAX_RELAYS both went 5 -> 6. Every NV item defined from
 * MAX_SWITCHES onward in nvm_items.h -- relay, cover switch, cover -- is
 * offset by one or both of them, so this is not a uniform shift:
 *
 *   - relay items move by +1 (MAX_SWITCHES alone)
 *   - cover-switch and cover items move by +2 (MAX_SWITCHES AND MAX_RELAYS)
 *
 * NV_ITEM_SWITCH_CLUSTER_DATA is NOT affected (its offset never included
 * either constant). migrate_item() computes each old/new pair from the v1
 * and current formulas independently rather than applying one shift amount,
 * so the different distances fall out correctly on their own.
 *
 * Copied highest item_id first: on every board this raises new_id above the
 * highest old_id that formula can produce, so writing new_id in that order
 * only ever clobbers an old_id already relocated in an earlier step of this
 * same migration. Byte counts are hardcoded rather than sizeof(their
 * structs) on purpose -- this migration has to keep reproducing exactly
 * what v1 actually wrote (4 / 5 / 1 bytes respectively, per
 * relay_cluster.c / cover_switch_cluster.c / cover_cluster.c's private
 * config structs at the time), independent of whatever those structs look
 * like after this. */
#define V1_MAX_SWITCHES 5
#define V1_MAX_RELAYS   5

#define V1_NV_ITEM_RELAY_CLUSTER_DATA(relay_idx) \
        (NV_ITEM_BASIC_CLUSTER_DATA + V1_MAX_SWITCHES + 1 + (relay_idx))
#define V1_NV_ITEM_COVER_SWITCH_CONFIG(cover_switch_idx)                        \
        (NV_ITEM_BASIC_CLUSTER_DATA + V1_MAX_SWITCHES + V1_MAX_RELAYS + 1 + \
         (cover_switch_idx))
#define V1_NV_ITEM_COVER_CONFIG(cover_idx)                                          \
        (NV_ITEM_BASIC_CLUSTER_DATA + V1_MAX_SWITCHES + V1_MAX_RELAYS +         \
         MAX_COVER_SWITCHES + 1 + (cover_idx))

static void migrate_item(uint8_t old_id, uint8_t new_id, uint16_t size) {
    uint8_t buf[5]; // widest struct moved here (cover switch config, 5 bytes)
    if (hal_nvm_read(old_id, size, buf) == HAL_NVM_SUCCESS) {
        hal_nvm_write(new_id, size, buf);
    }
    // Nothing at old_id: this board never had that many relays/covers.
    // Leave new_id unwritten -- the runtime read at boot already treats a
    // missing item as "no saved config" the same way it always has.
}

static void migrate_v1_to_v2_max_6() {
    // Highest item_id block first (covers), then cover switches, then
    // relays -- see the block comment above for why the order matters.
    //
    // Every loop bound below is the OLD (v1) count, not the current one:
    // this only relocates data that actually exists at an old_id. MAX_COVER_
    // SWITCHES/MAX_COVERS did not change, so their v1 count is the same
    // constant: V1_MAX_COVER_SWITCHES/V1_MAX_COVERS would just alias them.
    // MAX_RELAYS DID change (5 -> 6): looping to the new MAX_RELAYS - 1
    // here would compute a V1_NV_ITEM_RELAY_CLUSTER_DATA(5) that lands on
    // the old cover-switch region instead (relay item 5 never existed in
    // v1), reading and relocating the wrong data into the new slot 5.
    for (int i = MAX_COVERS - 1; i >= 0; i--) {
        migrate_item(V1_NV_ITEM_COVER_CONFIG(i), NV_ITEM_COVER_CONFIG(i), 1);
    }
    for (int i = MAX_COVER_SWITCHES - 1; i >= 0; i--) {
        migrate_item(V1_NV_ITEM_COVER_SWITCH_CONFIG(i), NV_ITEM_COVER_SWITCH_CONFIG(i), 5);
    }
    for (int i = V1_MAX_RELAYS - 1; i >= 0; i--) {
        migrate_item(V1_NV_ITEM_RELAY_CLUSTER_DATA(i), NV_ITEM_RELAY_CLUSTER_DATA(i), 4);
    }
    // New relay slot 5 (the 6th relay) has no v1 data to migrate -- it did
    // not exist before this change. It reads back as "no saved config" at
    // boot, the same as any other never-configured item.
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

    // Handle migrations here. Each one guarded by "< N" so a device that
    // skips several versions in one OTA runs all of them in order.
    if (oldVersion < 2) {
        migrate_v1_to_v2_max_6();
    }

    // Must run after every migration above: leaving this out means a
    // device that took the migration path never converges to
    // currentVersion, and re-runs every migration again on the next boot.
    write_version_to_nv(currentVersion);
}
