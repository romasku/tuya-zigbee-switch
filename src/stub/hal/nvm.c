#include "hal/nvm.h"
#include "stub/machine_io.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_NVM_ITEMS    256
#define NVM_DATA_DIR     "./stub_nvm_data"

static void ensure_nvm_dir(void) {
    struct stat st = { 0 };

    if (stat(NVM_DATA_DIR, &st) == -1) {
        if (mkdir(NVM_DATA_DIR, 0700) != 0) {
            io_log("NVM", "Error: Failed to create NVM directory: %s", NVM_DATA_DIR);
            exit(1);
        }
        io_log("NVM", "Created NVM directory: %s", NVM_DATA_DIR);
    }
}

static char *get_item_filename(uint8_t item_id) {
    static char filename[256];

    snprintf(filename, sizeof(filename), "%s/item_%02x.bin", NVM_DATA_DIR,
             item_id);
    return filename;
}

hal_nvm_status_t hal_nvm_write(uint8_t item_id, uint16_t size, uint8_t *data) {
    if (!data) {
        io_log("NVM",
               "Error: NULL data pointer passed to hal_nvm_write for item %02x",
               item_id);
        return HAL_NVM_ERROR;
    }

    if (size == 0) {
        io_log("NVM", "Error: Zero size passed to hal_nvm_write for item %02x",
               item_id);
        return HAL_NVM_ERROR;
    }

    ensure_nvm_dir();

    char *filename = get_item_filename(item_id);
    FILE *file     = fopen(filename, "wb");
    if (!file) {
        io_log("NVM", "Failed to open file for writing: %s", filename);
        return HAL_NVM_ERROR;
    }

    size_t written = fwrite(data, 1, size, file);
    fclose(file);

    if (written != size) {
        io_log("NVM", "Failed to write all data for item %02x", item_id);
        return HAL_NVM_ERROR;
    }

    io_log("NVM", "Wrote %d bytes to item %02x", size, item_id);
    return HAL_NVM_SUCCESS;
}

hal_nvm_status_t hal_nvm_read(uint8_t item_id, uint16_t size, uint8_t *data) {
    if (!data)
        return HAL_NVM_ERROR;

    char *filename = get_item_filename(item_id);
    FILE *file     = fopen(filename, "rb");
    if (!file) {
        io_log("NVM", "Item %02x not found", item_id);
        return HAL_NVM_NOT_FOUND;
    }

    size_t read_bytes = fread(data, 1, size, file);
    fclose(file);

    if (read_bytes != size) {
        io_log("NVM", "Read %zu bytes instead of %d for item %02x", read_bytes,
               size, item_id);
        return HAL_NVM_ERROR;
    }

    io_log("NVM", "Read %d bytes from item %02x", size, item_id);
    return HAL_NVM_SUCCESS;
}

hal_nvm_status_t hal_nvm_delete(uint8_t item_id) {
    char *filename = get_item_filename(item_id);

    if (unlink(filename) != 0) {
        // Check if the error is because the file doesn't exist
        if (errno == ENOENT) {
            io_log("NVM", "Item %02x not found for deletion", item_id);
            return HAL_NVM_NOT_FOUND;
        } else {
            io_log("NVM", "Failed to delete item %02x", item_id);
            return HAL_NVM_ERROR;
        }
    }

    io_log("NVM", "Deleted item %02x", item_id);
    return HAL_NVM_SUCCESS;
}

hal_nvm_status_t hal_nvm_clear_all() {
    ensure_nvm_dir();

    // Remove all files in the NVM directory
    char command[512];
    snprintf(command, sizeof(command), "rm -f %s/*", NVM_DATA_DIR);

    if (system(command) != 0) {
        io_log("NVM", "Failed to clear all NVM items");
        return HAL_NVM_ERROR;
    }

    io_log("NVM", "Cleared all NVM items");
    return HAL_NVM_SUCCESS;
}

void stub_nvm_set_data_dir(const char *dir) {
    // For testing, allow changing the data directory
    // This is not thread-safe but sufficient for testing
    static char custom_dir[256];

    strncpy(custom_dir, dir, sizeof(custom_dir) - 1);
    custom_dir[sizeof(custom_dir) - 1] = '\0';
}
