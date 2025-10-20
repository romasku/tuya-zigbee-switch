# Project configuration

PROJECT_ROOT := ../..
BUILD_DIR := $(PROJECT_ROOT)/build/telink
BIN_DIR := $(BUILD_DIR)/bin
TELINK_TOOLS_DIR := $(PROJECT_ROOT)/telink_tools
SRC_DIR := $(PROJECT_ROOT)/src

# Version and board configuration
VERSION ?= 20
FIRMWARE_BASENAME ?= tlc_switch
DEVICE_TYPE ?= router
CONFIG_STR ?= jl7qyupf;TS0012-custom;BA0f;LD7;SC2f;RC0;SC3f;RB4;
MANUFACTURER_ID ?= 4417
IMAGE_TYPE ?= 54179
OTA_VERSION ?= ${VERSION}
OTA_MANUFACTURER_ID ?= ${MANUFACTURER_ID}
OTA_IMAGE_TYPE ?= ${IMAGE_TYPE}

# Toolchain paths
SDK_PATH := $(TELINK_TOOLS_DIR)/sdk
TOOLCHAIN_PATH := $(TELINK_TOOLS_DIR)/toolchain/tc32
TC32_PATH := $(TOOLCHAIN_PATH)/bin

# Compiler and linker
CC := $(TC32_PATH)/tc32-elf-gcc
LD := $(TC32_PATH)/tc32-elf-ld
OBJCOPY := $(TC32_PATH)/tc32-elf-objcopy
OBJDUMP := $(TC32_PATH)/tc32-elf-objdump
SIZE := $(TC32_PATH)/tc32-elf-size

# Flash tool configuration
TLSRPGM_TTY ?= /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
TLSRPGM_TOOL := $(TELINK_TOOLS_DIR)/tlsrpgm/TlsrPgm.py

# Chip and library configuration with Zigbee libraries
ifeq ($(DEVICE_TYPE), router)
	TEL_CHIP := -DMCU_CORE_8258=1 -DROUTER=1 -DMCU_STARTUP_8258=1
	LIBS := -ldrivers_8258 -lzb_router
endif

ifeq ($(DEVICE_TYPE), end_device)
	TEL_CHIP := -DMCU_CORE_8258=1 -DEND_DEVICE=1 -DMCU_STARTUP_8258=1 -DPM_ENABLE
	LIBS := -ldrivers_8258 -lzb_ed
endif

# Compiler and linker flags
GCC_FLAGS := \
	-O2 \
	-ffunction-sections \
	-fdata-sections \
	-Wall \
	-fpack-struct \
	-fshort-enums \
	-finline-small-functions \
	-std=gnu99 \
	-funsigned-char \
	-fshort-wchar \
	-fms-extensions \
	-nostartfiles \
	-nostdlib \
	-MMD \
	-MP

LNK_FLAGS := --gc-sections -nostartfiles 

ASM_FLAGS := \
	-fomit-frame-pointer \
	-fshort-enums \
	-Wall \
	-Wpacked \
	-Wcast-align \
	-fdata-sections \
	-ffunction-sections \
	-fno-use-cxa-atexit \
	-fno-rtti \
	-fno-threadsafe-statics

# Device definitions
DEVICE_DEFS := \
	-DSTACK_BUILD=$(VERSION) \
	-DDEFAULT_CONFIG="$(CONFIG_STR)" \
	-DIMAGE_TYPE=$(IMAGE_TYPE) \
	-DHAL_TELINK \
	$(TEL_CHIP) 

# Include paths (SDK paths first to avoid conflicts)
INCLUDE_PATHS := \
	-I. \
	-Iconfigs \
	-Ilibc_polyfills \
	-Icustom_zcl \
	-I$(SRC_DIR) \
	-I$(SDK_PATH)/platform \
	-I$(SDK_PATH)/platform/chip_8258 \
	-I$(SDK_PATH)/proj \
	-I$(SDK_PATH)/proj/common \
	-I$(SDK_PATH)/zigbee \
	-I$(SDK_PATH)/zigbee/af \
	-I$(SDK_PATH)/zigbee/include \
	-I$(SDK_PATH)/zigbee/bdb/includes \
	-I$(SDK_PATH)/zigbee/common/includes \
	-I$(SDK_PATH)/zigbee/ota \
	-I$(SDK_PATH)/zigbee/zbapi \
	-I$(SDK_PATH)/zigbee/zbhci \
	-I$(SDK_PATH)/zigbee/zcl \
	-I$(SDK_PATH)/zigbee/zdo

# Library paths
LIBRARY_PATHS := -L$(SDK_PATH)/zigbee/lib/tc32 -L$(SDK_PATH)/platform/lib

# Source files
TELINK_SOURCES := \
	main.c \
	ota_reformating/ensure_ota_scheme.c \
	ota_reformating/ram_code_flash.c \
	hal/system.c \
	hal/timer.c \
	hal/tasks.c \
	hal/gpio.c \
	hal/gpio_interrupts.c \
	hal/nvm.c \
	hal/zigbee.c \
	hal/zigbee_network.c \
	hal/zigbee_zcl.c \
	hal/zigbee_ota.c \
	libc_polyfills/atoi.c \
	custom_zcl/zcl_multistate_input.c \
	custom_zcl/zcl_onoff_configuration.c

# Common source files (shared with Silicon Labs build)
COMMON_SOURCES := \
	$(SRC_DIR)/app.c \
	$(SRC_DIR)/base_components/button.c \
	$(SRC_DIR)/base_components/led.c \
	$(SRC_DIR)/base_components/network_indicator.c \
	$(SRC_DIR)/base_components/relay.c \
	$(SRC_DIR)/device_config/config_nv.c \
	$(SRC_DIR)/device_config/config_parser.c \
	$(SRC_DIR)/zigbee/basic_cluster.c \
	$(SRC_DIR)/zigbee/general_commands.c \
	$(SRC_DIR)/zigbee/group_cluster.c \
	$(SRC_DIR)/zigbee/relay_cluster.c \
	$(SRC_DIR)/zigbee/switch_cluster.c

# All application sources
ALL_TELINK_SOURCES := $(TELINK_SOURCES) $(COMMON_SOURCES)

# Include SDK build rules
include sdk.mk

# Application object files  
TELINK_OBJS := $(TELINK_SOURCES:%.c=$(BUILD_DIR)/telink/%.o)
COMMON_OBJS := $(COMMON_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/common/%.o)
ALL_APP_OBJS := $(TELINK_OBJS) $(COMMON_OBJS)

# Dependency files for application sources
TELINK_DEPS := $(TELINK_SOURCES:%.c=$(BUILD_DIR)/telink/%.d)
COMMON_DEPS := $(COMMON_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/common/%.d)
ALL_APP_DEPS := $(TELINK_DEPS) $(COMMON_DEPS)

# All object files (application + common + SDK)
ALL_OBJS := $(ALL_APP_OBJS) $(SDK_OBJS)

# Linker script
LINKER_SCRIPT := $(SDK_PATH)/platform/boot/8258/boot_8258.link

# Output files
ELF_FILE := $(BUILD_DIR)/$(FIRMWARE_BASENAME).elf
LST_FILE := $(BUILD_DIR)/$(FIRMWARE_BASENAME).lst
BIN_FILE := $(BIN_DIR)/$(FIRMWARE_BASENAME).bin
OTA_FILE := $(BIN_DIR)/$(FIRMWARE_BASENAME).ota
SIZEDUMMY := $(BUILD_DIR)/sizedummy

# Default target
all: $(BIN_FILE) $(LST_FILE)
	@echo "Build complete: $(BIN_FILE)"

# Build main ELF file
$(ELF_FILE): $(ALL_APP_OBJS) sdk-build
	@echo "Linking $(ELF_FILE)"
	@$(LD) $(LNK_FLAGS) -T$(LINKER_SCRIPT) $(LIBRARY_PATHS) -o $@ $(ALL_APP_OBJS) $(SDK_OBJS) $(LIBS)


$(LST_FILE): $(ELF_FILE)
	@echo 'Invoking: TC32 Create Extended Listing'
	@$(OBJDUMP) -x -D -l -S  $(ELF_FILE)  > $(LST_FILE)

# Create binary file
$(BIN_FILE): $(ELF_FILE)
	@echo "Creating binary $(BIN_FILE)"
	@mkdir -p $(dir $@)
	@$(OBJCOPY) -O binary $< $@
	@$(SIZE) $<

# Compile Telink-specific C files
$(BUILD_DIR)/telink/%.o: %.c
	@echo "Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(GCC_FLAGS) $(DEVICE_DEFS) $(INCLUDE_PATHS) -c -o $@ $<

# Compile common C files
$(BUILD_DIR)/common/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(GCC_FLAGS) $(DEVICE_DEFS) $(INCLUDE_PATHS) -c -o $@ $<

# SDK compilation rules are in sdk.mk


# Build with tools check
build: all

# Make an ota file
ota: $(BIN_FILE)
	@echo 'Creating OTA file $(OTA_FILE)'
	@echo ' '
	@mkdir -p $(dir $@)
	@python3 $(SRC_DIR)/telink/make_ota.py create-ota \
	  --manufacturer-id $(OTA_MANUFACTURER_ID) \
	  --image-type $(OTA_IMAGE_TYPE) \
	  --file-version $(OTA_VERSION) \
	  $(BIN_FILE) $(OTA_FILE)
	@echo ' '

# Flash binary to device
flash: $(BIN_FILE)
	@echo "Flashing $(BIN_FILE) to device via $(TLSRPGM_TTY)"
	@python3 $(TLSRPGM_TOOL) -p$(TLSRPGM_TTY) -t25 -a 20 --mrst we 0 $(BIN_FILE)

# Restart device
restart:
	@echo "Restarting device via $(TLSRPGM_TTY)"
	@python3 $(TLSRPGM_TOOL) -p$(TLSRPGM_TTY)  -t25 -a 20 --mrst

# Wipe device
wipe: 
	@echo "Wiping device via $(TLSRPGM_TTY)"
	@python3 $(TLSRPGM_TOOL) -p$(TLSRPGM_TTY) -t25 -a 20 ea

flasher:
	@python3 $(TLSRPGM_TOOL) -p$(TLSRPGM_TTY) $(ARGS)

# Include dependency files (if they exist)
-include $(ALL_APP_DEPS)

# Clean build artifacts
clean: sdk-clean
	@echo "Cleaning Telink build artifacts..."
	@rm -rf $(BUILD_DIR)/telink
	@rm -rf $(BUILD_DIR)/common
	@rm -rf $(BIN_DIR)


# Delegate tools targets to tools.mk
tools/%:
	$(MAKE) -f tools.mk $*

# Help target
help:
	@echo "Telink Build System"
	@echo "=================="
	@echo ""
	@echo "Tools Setup Targets:"
	@echo "  tools/all          - Download and install all Telink tools"
	@echo "  tools/status       - Show installation status and verify tools"
	@echo "  tools/verify       - Verify installed tools"
	@echo "  tools/help         - Show detailed tools help"
	@echo ""
	@echo "Quick Start:"
	@echo "  make build          - Check tools and build firmware"
	@echo "  make all            - Build firmware (assumes tools installed)"
	@echo "  make install-tools  - Install Telink SDK and toolchain (deprecated, use tools/all)"
	@echo ""
	@echo "Configuration:"
	@echo "  BOARD=<board>       - Target board (default: $(BOARD))"
	@echo "  DEVICE_TYPE=<type>  - router or end_device (default: $(DEVICE_TYPE))"
	@echo "  DEBUG=1             - Enable debug output (default: $(DEBUG))"
	@echo ""
	@echo "Build Targets:"
	@echo "  build               - Build with tools verification"
	@echo "  all                 - Build firmware"
	@echo "  clean               - Clean build artifacts"
	@echo "  sdk-build           - Build SDK sources only"
	@echo "  sdk-clean           - Clean SDK build artifacts"
	@echo "  sdk-help            - Show SDK build help"
	@echo "  check-tools         - Verify development tools"
	@echo "  install-tools       - Install development tools"
	@echo "  help                - Show this help"
	@echo ""
	@echo "Output:"
	@echo "  ELF: $(ELF_FILE)"
	@echo "  BIN: $(BIN_FILE)"

.PHONY: all build clean check-tools install-tools help directories