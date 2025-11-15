# ==============================================================================
# Configuration Variables
# ==============================================================================

# Help target
help:
	@echo "Board-Specific Build System"
	@echo "==========================="
	@echo ""
	@echo "Device Database Build:"
	@echo "  build              - Build firmware for specified BOARD (from device_db.yaml)"
	@echo ""
	@echo "Configuration:"
	@echo "  BOARD              - Device name from device_db.yaml (default: $(BOARD))"
	@echo "  DEVICE_TYPE        - Extracted from database (current: $(DEVICE_TYPE))"
	@echo ""
	@echo "Generated Files:"
	@echo "  OTA Files          - Standard, Tuya migration, and force upgrade variants"
	@echo "  Z2M Indexes        - Zigbee2MQTT OTA index updates"
	@echo ""
	@echo "Example Usage:"
	@echo "  BOARD=TUYA_TS0012 make build"
	@echo "  BOARD=MOES_3_GANG_SWITCH DEVICE_TYPE=router make build"
	@echo ""

# Parse version from VERSION file
VERSION_FILE_CONTENT := $(shell cat VERSION 2>/dev/null || echo "1.0.0")

# Split semantic version
VERSION_PARTS := $(subst ., ,$(VERSION_FILE_CONTENT))
VERSION_MAJOR := $(word 1,$(VERSION_PARTS))
VERSION_MINOR := $(word 2,$(VERSION_PARTS))
VERSION_PATCH := $(word 3,$(VERSION_PARTS))

# Get git info
GIT_HASH   := $(shell git rev-parse --short HEAD 2>/dev/null || echo "")

# Limites to 16 characters
VERSION_STR := $(shell echo "$(VERSION_FILE_CONTENT)-$(GIT_HASH)" | cut -c1-16)

# Zigbee file version 4 bytes are:
# APP_RELEASE, APP_BUILD, STACK_RELEASE, STACK_BUILD
# We encode MAJOR.MINOR -> APP_RELEASE (in BCD format)
# PATCH -> APP_BUILD
# and STACK_RELEASE fixed to 0x30,
# STACK_BUILD is depth from main branch, to allow easy updating during feature development
STACK_BUILD := $(shell git rev-list --count origin/main..HEAD 2>/dev/null || echo "0")
VERSION_PATCH_HEX := $(shell printf "%02d" $(VERSION_PATCH))
STACK_BUILD_HEX := $(shell printf "%02X" $(STACK_BUILD))
FILE_VERSION = 0x$(VERSION_MAJOR)$(VERSION_MINOR)$(VERSION_PATCH_HEX)30$(STACK_BUILD_HEX)

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
ifeq ($(PLATFORM_PREFIX),silabs)
BIN_FILE := $(BIN_PATH)/$(PROJECT_NAME)-$(VERSION_STR).s37
else
BIN_FILE := $(BIN_PATH)/$(PROJECT_NAME)-$(VERSION_STR).bin
endif
OTA_FILE := $(BIN_PATH)/$(PROJECT_NAME)-$(VERSION_STR).zigbee
FROM_TUYA_OTA_FILE := $(BIN_PATH)/$(PROJECT_NAME)-$(VERSION_STR)-from_tuya.zigbee
FORCE_OTA_FILE := $(BIN_PATH)/$(PROJECT_NAME)-$(VERSION_STR)-forced.zigbee

# Index Files
Z2M_INDEX_FILE := zigbee2mqtt/ota/index_$(DEVICE_TYPE).json
Z2M_FORCE_INDEX_FILE := zigbee2mqtt/ota/index_$(DEVICE_TYPE)-FORCE.json

# Main target - builds firmware and generates all OTA files
build: drop-old-files build-firmware generate-ota-files update-indexes

# Build the firmware for the specified board
build-firmware:
ifeq ($(PLATFORM_PREFIX),silabs)
	$(MAKE) silabs/gen
endif
ifeq ($(PLATFORM_PREFIX),telink)
	$(MAKE) telink/clean
endif
	$(MAKE) $(PLATFORM_PREFIX)/build \
		VERSION_STR=$(VERSION_STR) \
		FILE_VERSION=$(FILE_VERSION) \
		DEVICE_TYPE=$(DEVICE_TYPE) \
		CONFIG_STR="$(CONFIG_STR)" \
		IMAGE_TYPE=$(FIRMWARE_IMAGE_TYPE) \
		BIN_FILE=../../$(BIN_FILE) \
		 -j32

drop-old-files:
	rm -f $(BIN_PATH)/*.bin
	rm -f $(BIN_PATH)/*.s37
	rm -f $(BIN_PATH)/*.zigbee

# Generate all three types of OTA files
generate-ota-files: generate-normal-ota generate-tuya-ota generate-force-ota

generate-normal-ota:
	$(MAKE) $(PLATFORM_PREFIX)/ota \
		DEVICE_TYPE=$(DEVICE_TYPE) \
		OTA_IMAGE_TYPE=$(FIRMWARE_IMAGE_TYPE) \
		OTA_FILE=../../$(OTA_FILE)

generate-tuya-ota:
ifneq ($(PLATFORM_PREFIX),silabs)  # Silabs platform does not support Tuya migration OTAs
	$(MAKE) $(PLATFORM_PREFIX)/ota \
		OTA_VERSION=0xFFFFFFFF \
		DEVICE_TYPE=$(DEVICE_TYPE) \
		OTA_IMAGE_TYPE=$(FROM_TUYA_IMAGE_TYPE) \
		OTA_MANUFACTURER_ID=$(FROM_TUYA_MANUFACTURER_ID) \
		OTA_FILE=../../$(FROM_TUYA_OTA_FILE)
endif

generate-force-ota:
	$(MAKE) $(PLATFORM_PREFIX)/ota \
		OTA_VERSION=0xFFFFFFFF \
		DEVICE_TYPE=$(DEVICE_TYPE) \
		OTA_IMAGE_TYPE=$(FIRMWARE_IMAGE_TYPE) \
		OTA_FILE=../../$(FORCE_OTA_FILE)

# Update Zigbee2MQTT index files
update-indexes:
	@python3 $(HELPERS_PATH)/make_z2m_ota_index.py --db_file $(DEVICE_DB_FILE) $(OTA_FILE) $(Z2M_INDEX_FILE) --board $(BOARD)
ifneq ($(PLATFORM_PREFIX),silabs)  # Silabs platform does not support Tuya migration OTAs
	@python3 $(HELPERS_PATH)/make_z2m_ota_index.py --db_file $(DEVICE_DB_FILE) $(FROM_TUYA_OTA_FILE) $(Z2M_INDEX_FILE) --board $(BOARD)
endif
	@python3 $(HELPERS_PATH)/make_z2m_ota_index.py --db_file $(DEVICE_DB_FILE) $(FORCE_OTA_FILE) $(Z2M_FORCE_INDEX_FILE) --board $(BOARD)


flash_telink: build-firmware
	@echo "Flashing $(BIN_FILE) to device via $(TLSRPGM_TTY)"
	$(MAKE) telink/flasher ARGS="-t25 -a 20 --mrst we 0 ../../$(BIN_FILE)"

.PHONY: help build build-firmware drop-old-files generate-ota-files generate-normal-ota generate-tuya-ota generate-force-ota update-indexes clean_z2m_index update_converters update_zha_quirk update_supported_devices freeze_ota_links

