# Project configuration

PROJECT_ROOT := ../..
BUILD_DIR := $(PROJECT_ROOT)/build/telink
BIN_DIR := $(BUILD_DIR)/bin
TELINK_TOOLS_DIR := $(PROJECT_ROOT)/telink_tools
SRC_DIR := $(PROJECT_ROOT)/src

# Version and board configuration
VERSION_STR ?= "0.0.0"
FILE_VERSION ?= 0

FIRMWARE_BASENAME ?= tlc_switch
DEVICE_TYPE ?= router
DEBUG ?= 0
CONFIG_STR ?= jl7qyupf;TS0012-custom;BA0f;LD7;SC2f;RC0;SC3f;RB4;
MANUFACTURER_ID ?= 4417
IMAGE_TYPE ?= 43521
OTA_VERSION ?= 
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
    -DVERSION_STR="$(VERSION_STR)" \
	-DFILE_VERSION=$(FILE_VERSION) \
	-DDEFAULT_CONFIG="$(CONFIG_STR)" \
	-DIMAGE_TYPE=$(IMAGE_TYPE) \
	-DHAL_TELINK \
	$(TEL_CHIP) 

ifeq ($(DEBUG), 1)
	DEVICE_DEFS := $(DEVICE_DEFS) -DUART_PRINTF_MODE=1
endif

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
	$(SRC_DIR)/device_config/reset.c \
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

ifdef OTA_VERSION
OTA_VERSION_ARG := --file-version $(OTA_VERSION)
endif

# Make an ota file
ota: $(BIN_FILE)
	@echo 'Creating OTA file $(OTA_FILE)'
	@echo ' '
	@mkdir -p $(dir $(OTA_FILE))
	@python3 $(SRC_DIR)/telink/make_ota.py create-ota \
	  --manufacturer-id $(OTA_MANUFACTURER_ID) \
	  --image-type $(OTA_IMAGE_TYPE) \
	  $(OTA_VERSION_ARG) \
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
	@echo ""
	@echo "Build Targets:"
	@echo "  build               - Build with tools verification"
	@echo "  all                 - Build firmware (ELF + BIN + LST files)"
	@echo "  ota                 - Create OTA update file from binary"
	@echo "  clean               - Clean build artifacts"
	@echo "  sdk-build           - Build SDK sources only"
	@echo "  sdk-clean           - Clean SDK build artifacts"
	@echo "  sdk-help            - Show SDK build help"
	@echo ""
	@echo "Hardware/Device Targets:"
	@echo "  flash               - Flash binary to device via programmer"
	@echo "  restart             - Restart device via programmer"
	@echo "  wipe                - Wipe device flash via programmer"
	@echo "  flasher             - Run programmer tool with custom ARGS"
	@echo ""
	@echo "Configuration Variables:"
	@echo "  APP_RELEASE         - Application release version (default: $(APP_RELEASE))"
	@echo "  APP_BUILD           - Application build version (default: $(APP_BUILD))"
	@echo "  STACK_RELEASE       - Stack release version (default: $(STACK_RELEASE))"
	@echo "  STACK_BUILD         - Stack build version (default: $(STACK_BUILD))"
	@echo "  DEVICE_TYPE         - router or end_device (default: $(DEVICE_TYPE))"
	@echo "  CONFIG_STR          - Device pin configuration string"
	@echo "  DEBUG               - Enable debug output (0/1, default: $(DEBUG))"
	@echo "  TLSRPGM_TTY         - Programmer serial port (default: $(TLSRPGM_TTY))"
	@echo ""
	@echo "Help Targets:"
	@echo "  help                - Show this help"
	@echo ""
	@echo "Output Files:"
	@echo "  ELF: $(ELF_FILE)"
	@echo "  BIN: $(BIN_FILE)"
	@echo "  LST: $(LST_FILE)"

.PHONY: all build clean ota flash restart wipe flasher help