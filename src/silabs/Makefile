# Silicon Labs build configuration
GSDK_VERSION       ?= v4.4.6
GSDK_DIR           ?= ../../silabs_tools/gecko_sdk
SLC_DIR            ?= ../../silabs_tools/slc-cli
COMMANDER_DIR      ?= ../../silabs_tools/commander
ZAP_DIR            ?= ../../silabs_tools/zap
GCC_BIN            ?= arm-none-eabi-gcc

# Version and board configuration
VERSION_STR ?= 0.0.0
FILE_VERSION ?= 0
DEBUG ?= 0


DEVICE_TYPE ?= router
CONFIG_STR ?= jl7qyupf;TS0012-custom;SA0u;SA3u;SB0u;SB1u;RD0;RC2;RA5;RA6;
MANUFACTURER_ID = 4169  # 0x1049, can be overridden via .zap file only
IMAGE_TYPE ?= 54179

EXTRA_C_DEFS = -DFILE_VERSION=$(FILE_VERSION) -DVERSION_STR="$(VERSION_STR)" -DDEFAULT_CONFIG="$(CONFIG_STR)" -DIMAGE_TYPE=$(IMAGE_TYPE) -DHAL_SILABS

# Default target
help:
	@echo "Silicon Labs EFR32 Zigbee Device Build System"
	@echo "============================================"
	@echo ""
	@echo "Tools Setup Targets:"
	@echo "  tools/all          - Download and install all Silicon Labs tools"
	@echo "  tools/trust        - Trust SDK signature (run after SDK installation)"
	@echo "  tools/verify       - Verify installed tools and system requirements"
	@echo "  tools/help         - Show detailed tools help"
	@echo ""
	@echo "Setup Targets:"
	@echo "  gen                - Generate project files from .slcp configuration"
	@echo ""
	@echo "Build Targets:"
	@echo "  build              - Build the Zigbee application firmware"
	@echo "  ota                - Create OTA upgrade file from built firmware"
	@echo ""
	@echo "Flash Targets:"
	@echo "  install            - Flash firmware to EFR32MG21 device"
	@echo ""
	@echo "Device Management:"
	@echo "  erase_dev          - Mass erase the device flash memory"
	@echo "  restart_dev        - Reset the device"
	@echo ""
	@echo "Bootloader Targets:"
	@echo "  bootloader_gen     - Generate bootloader project files"
	@echo "  bootloader_build   - Build the bootloader"
	@echo "  bootloader_install - Flash bootloader to device"
	@echo ""
	@echo "Usage Example:"
	@echo "  make tools/all     # First time: download all tools"
	@echo "  make tools/trust   # Trust SDK signature"
	@echo "  make gen           # Generate build files"
	@echo "  make build         # Compile firmware"
	@echo "  make install       # Flash to device"
	@echo ""

TOOLCHAIN_BIN := $(shell command -v $(GCC_BIN))
TOOLCHAIN_DIR := $(abspath $(dir $(TOOLCHAIN_BIN))/..)

# Delegate tools targets to tools.mk
tools/%:
	$(MAKE) -f tools.mk $*

# Build directories
ZIGBEE_BUILD_DIR := ../../build/silabs/zigbee
ZIGBEE_PROJECT_NAME := zigbee
ifeq ($(DEVICE_TYPE), router)
	PROJECT_FILE = zigbee.slcp
endif
ifeq ($(DEVICE_TYPE), end_device)
	PROJECT_FILE = zigbee_end_device.slcp
endif
BOOTLOADER_BUILD_DIR := ../../build/silabs/bootloader
BOOTLOADER_PROJECT_NAME := bootloader
BOOTLOADER_PROJECT_FILE := bootloader.slcp

# Files:
OTA_FILE ?= $(ZIGBEE_BUILD_DIR)/build/release/$(ZIGBEE_PROJECT_NAME)_ota.bin
BIN_FILE ?= $(ZIGBEE_BUILD_DIR)/build/release/$(ZIGBEE_PROJECT_NAME).s37

# Component exclusion based on DEBUG flag
ifeq ($(DEBUG), 0)
	WITHOUT_COMPONENTS = --without=iostream_usart:inst,iostream_recommended_stream
else
	WITHOUT_COMPONENTS =
endif

# Generate project files
gen:
	rm -rf $(ZIGBEE_BUILD_DIR)
	$(SLC_DIR)/slc configuration -gcc $(TOOLCHAIN_DIR)
	$(SLC_DIR)/slc configuration --sdk $(GSDK_DIR)

	@# Unset virtual environment variables to avoid conflicts and set ZAP environment
	env -u VIRTUAL_ENV -u PYTHONHOME -u PYTHONPATH \
		STUDIO_ADAPTER_PACK_PATH="$(abspath $(ZAP_DIR))" \
    PATH="$$(echo "$$PATH" | tr ':' '\n' | grep -v 'venv' | paste -sd: -)" \
	$(SLC_DIR)/slc generate \
		--project-file "$(PROJECT_FILE)" \
		-np -nocp \
		-d "$(ZIGBEE_BUILD_DIR)" \
		-name $(ZIGBEE_PROJECT_NAME) \
		--with EFR32MG21A020F768IM32 \
		$(WITHOUT_COMPONENTS) \
		--toolchain=gcc \
		--output-type=makefile
	
	# Add support for extra C defines
	@echo "" >> $(ZIGBEE_BUILD_DIR)/$(ZIGBEE_PROJECT_NAME).Makefile
	@echo "# Support for additional C defines" >> $(ZIGBEE_BUILD_DIR)/$(ZIGBEE_PROJECT_NAME).Makefile
	@echo "ifdef EXTRA_C_DEFS" >> $(ZIGBEE_BUILD_DIR)/$(ZIGBEE_PROJECT_NAME).Makefile
	@echo "  C_DEFS += \$$(EXTRA_C_DEFS)" >> $(ZIGBEE_BUILD_DIR)/$(ZIGBEE_PROJECT_NAME).Makefile
	@echo "endif" >> $(ZIGBEE_BUILD_DIR)/$(ZIGBEE_PROJECT_NAME).Makefile

# Build targets
build:
	cd $(ZIGBEE_BUILD_DIR) && make -f $(ZIGBEE_PROJECT_NAME).Makefile -j32 release EXTRA_C_DEFS='$(EXTRA_C_DEFS)'
	@if ! cmp -s $(ZIGBEE_BUILD_DIR)/build/release/$(ZIGBEE_PROJECT_NAME).s37 $(BIN_FILE); then \
		mkdir -p $(dir $(BIN_FILE)); \
		cp $(ZIGBEE_BUILD_DIR)/build/release/$(ZIGBEE_PROJECT_NAME).s37 $(BIN_FILE); \
	fi


# Flash targets
install:
	$(COMMANDER_DIR)/commander-cli flash --device EFR32MG21A020F768IM32 $(ZIGBEE_BUILD_DIR)/build/release/$(ZIGBEE_PROJECT_NAME).s37

# Device management
erase_dev:
	$(COMMANDER_DIR)/commander-cli device --device EFR32MG21A020F768IM32 masserase

restart_dev:
	$(COMMANDER_DIR)/commander-cli device --device EFR32MG21A020F768IM32 reset

ota:
	@mkdir -p $(dir $(OTA_FILE))
	$(COMMANDER_DIR)/commander-cli gbl create \
		"$(ZIGBEE_BUILD_DIR)/build/release/$(ZIGBEE_PROJECT_NAME).gbl" \
		--app "$(ZIGBEE_BUILD_DIR)/build/release/$(ZIGBEE_PROJECT_NAME).s37" \
		--compress lzma
	$(COMMANDER_DIR)/commander-cli ota create \
		--upgrade-image "$(ZIGBEE_BUILD_DIR)/build/release/$(ZIGBEE_PROJECT_NAME).gbl" \
		--firmware-version "$(FILE_VERSION)" \
		--manufacturer-id "${MANUFACTURER_ID}" \
		--image-type "$(IMAGE_TYPE)" \
		--string "Custom zigbee firmware" \
		--outfile "$(OTA_FILE)"
	

# Bootloader targets
bootloader_gen:
	rm -rf $(BOOTLOADER_BUILD_DIR)

	@# Unset virtual environment variables to avoid conflicts 
	env -u VIRTUAL_ENV -u PYTHONHOME -u PYTHONPATH \
    PATH="$$(echo "$$PATH" | tr ':' '\n' | grep -v 'venv' | paste -sd: -)" \
	$(SLC_DIR)/slc generate \
		--project-file "$(BOOTLOADER_PROJECT_FILE)" \
		-np -nocp \
		-d "$(BOOTLOADER_BUILD_DIR)" \
		-name $(BOOTLOADER_PROJECT_NAME) \
		--with EFR32MG21A020F768IM32 \
		--toolchain=gcc \
		--output-type=makefile

bootloader_build: bootloader_gen
	cd $(BOOTLOADER_BUILD_DIR) && make -f $(BOOTLOADER_PROJECT_NAME).Makefile -j32 release

bootloader_install:
	@echo "Flashing bootloader..."
	$(COMMANDER_DIR)/commander-cli flash $(BOOTLOADER_BUILD_DIR)/build/release/$(BOOTLOADER_PROJECT_NAME).s37 --device EFR32MG21A020F768IM32

.PHONY: help tools/% gen build build_release install install_release erase_dev restart_dev bootloader_gen bootloader_build bootloader_install