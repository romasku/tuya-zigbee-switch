# ==============================================================================
# Configuration Variables
# ==============================================================================
VERSION := 21
PROJECT_NAME := tlc_switch
BOARD ?= TUYA_TS0012
DEVICE_DB_FILE := device_db.yaml

# ==============================================================================
# Database-Derived Variables
# ==============================================================================
DEVICE_TYPE ?= $(shell yq -r .$(BOARD).device_type $(DEVICE_DB_FILE))
MCU ?= $(shell yq -r .$(BOARD).mcu $(DEVICE_DB_FILE))
CONFIG_STR := $(shell yq -r .$(BOARD).config_str $(DEVICE_DB_FILE))
FROM_TUYA_MANUFACTURER_ID := $(shell yq -r .$(BOARD).tuya_manufacturer_id $(DEVICE_DB_FILE))
FROM_TUYA_IMAGE_TYPE := $(shell yq -r .$(BOARD).tuya_image_type $(DEVICE_DB_FILE))
FIRMWARE_IMAGE_TYPE := $(shell yq -r .$(BOARD).firmware_image_type $(DEVICE_DB_FILE))

# ==============================================================================
# Platform Configuration
# ==============================================================================
PLATFORM_PREFIX := $(if $(filter TLSR8258,$(MCU)),telink,silabs)

# ==============================================================================
# Path Variables
# ==============================================================================
BOARD_DIR := $(BOARD)$(if $(filter end_device,$(DEVICE_TYPE)),_END_DEVICE)
BIN_PATH := bin/$(DEVICE_TYPE)/$(BOARD_DIR)
HELPERS_PATH := ./helper_scripts

# OTA Files
OTA_FILE := $(BIN_PATH)/$(PROJECT_NAME)-$(VERSION).zigbee
FROM_TUYA_OTA_FILE := $(BIN_PATH)/$(PROJECT_NAME)-$(VERSION)-from_tuya.zigbee
FORCE_OTA_FILE := $(BIN_PATH)/$(PROJECT_NAME)-$(VERSION)-forced.zigbee

# Index Files
Z2M_INDEX_FILE := zigbee2mqtt/ota/index_$(DEVICE_TYPE).json
Z2M_FORCE_INDEX_FILE := zigbee2mqtt/ota/index_$(DEVICE_TYPE)-FORCE.json

# Main target - builds firmware and generates all OTA files
board: build-firmware generate-ota-files update-indexes

# Build the firmware for the specified board
build-firmware:
ifeq ($(PLATFORM_PREFIX),silabs)
	$(MAKE) silabs/gen
endif
ifeq ($(PLATFORM_PREFIX),telink)
	$(MAKE) telink/clean
endif
	$(MAKE) $(PLATFORM_PREFIX)/build \
		VERSION=$(VERSION) \
		DEVICE_TYPE=$(DEVICE_TYPE) \
		CONFIG_STR="$(CONFIG_STR)" \
		IMAGE_TYPE=$(FIRMWARE_IMAGE_TYPE) -j32

# Generate all three types of OTA files
generate-ota-files: generate-normal-ota generate-tuya-ota generate-force-ota

generate-normal-ota:
	$(MAKE) $(PLATFORM_PREFIX)/ota \
		OTA_VERSION=$(VERSION) \
		DEVICE_TYPE=$(DEVICE_TYPE) \
		OTA_IMAGE_TYPE=$(FIRMWARE_IMAGE_TYPE) \
		OTA_FILE=../../$(OTA_FILE)

generate-tuya-ota:
	$(MAKE) $(PLATFORM_PREFIX)/ota \
		OTA_VERSION=0xFFFFFFFF \
		DEVICE_TYPE=$(DEVICE_TYPE) \
		OTA_IMAGE_TYPE=$(FROM_TUYA_IMAGE_TYPE) \
		OTA_MANUFACTURER_ID=$(FROM_TUYA_MANUFACTURER_ID) \
		OTA_FILE=../../$(FROM_TUYA_OTA_FILE)

generate-force-ota:
	$(MAKE) $(PLATFORM_PREFIX)/ota \
		OTA_VERSION=0xFFFFFFFF \
		DEVICE_TYPE=$(DEVICE_TYPE) \
		OTA_IMAGE_TYPE=$(FIRMWARE_IMAGE_TYPE) \
		OTA_FILE=../../$(FORCE_OTA_FILE)

# Update Zigbee2MQTT index files
update-indexes:
	@python3 $(HELPERS_PATH)/make_z2m_ota_index.py --db_file $(DEVICE_DB_FILE) $(OTA_FILE) $(Z2M_INDEX_FILE) --board $(BOARD)
	@python3 $(HELPERS_PATH)/make_z2m_ota_index.py --db_file $(DEVICE_DB_FILE) $(FROM_TUYA_OTA_FILE) $(Z2M_INDEX_FILE) --board $(BOARD)
	@python3 $(HELPERS_PATH)/make_z2m_ota_index.py --db_file $(DEVICE_DB_FILE) $(FORCE_OTA_FILE) $(Z2M_FORCE_INDEX_FILE) --board $(BOARD)


.PHONY: board build-firmware generate-ota-files generate-normal-ota generate-tuya-ota generate-force-ota update-indexes clean_z2m_index update_converters update_zha_quirk update_supported_devices freeze_ota_links


