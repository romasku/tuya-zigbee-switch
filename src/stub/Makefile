# Stub build configuration
HOST_CC            ?= gcc
SRC_DIR            := ..
BUILD_DIR          := ../../build/stub
BINARY             := $(BUILD_DIR)/stub_device

# Default target
help:
	@echo "Stub Device Build System"
	@echo "========================"
	@echo ""
	@echo "Build Targets:"
	@echo "  build              - Compile the stub device binary for host testing"
	@echo ""
	@echo "Run Targets:"
	@echo "  run                - Build and run the stub device in interactive mode"
	@echo ""
	@echo "Cleanup:"
	@echo "  clean              - Remove build artifacts and NVM data"
	@echo ""
	@echo "About:"
	@echo "  The stub device provides a host-native simulation environment"
	@echo "  for testing Zigbee device functionality without hardware."
	@echo "  It includes a REPL interface and Python test integration."
	@echo ""
	@echo "Usage Example:"
	@echo "  make build         # Compile stub device"
	@echo "  make run           # Run interactively"
	@echo ""

# Source files for stub build
SOURCES := \
	$(SRC_DIR)/app.c \
	$(SRC_DIR)/stub/main.c \
	$(SRC_DIR)/stub/hal/gpio.c \
	$(SRC_DIR)/stub/hal/system.c \
	$(SRC_DIR)/stub/hal/timer.c \
	$(SRC_DIR)/stub/hal/tasks.c \
	$(SRC_DIR)/stub/hal/nvm.c \
	$(SRC_DIR)/stub/hal/zigbee.c \
	$(SRC_DIR)/stub/hal/ota.c \
	$(SRC_DIR)/stub/simple_repl.c \
	$(SRC_DIR)/stub/stub_app.c \
	$(SRC_DIR)/stub/commands.c \
	$(SRC_DIR)/stub/machine_io.c \
	$(SRC_DIR)/base_components/led.c \
	$(SRC_DIR)/base_components/relay.c \
	$(SRC_DIR)/base_components/button.c \
	$(SRC_DIR)/base_components/network_indicator.c \
	$(SRC_DIR)/device_config/config_parser.c \
	$(SRC_DIR)/device_config/config_nv.c \
	$(SRC_DIR)/device_config/reset.c \
	$(SRC_DIR)/zigbee/basic_cluster.c \
	$(SRC_DIR)/zigbee/relay_cluster.c \
	$(SRC_DIR)/zigbee/switch_cluster.c \
	$(SRC_DIR)/zigbee/group_cluster.c \
	$(SRC_DIR)/zigbee/general_commands.c

INCLUDES := \
	-I$(SRC_DIR) \
	-I$(SRC_DIR)/include

CFLAGS := -Wall -Wno-unused-parameter -Wno-unused-variable -g -O0 \
    -DHAL_STUB -DSTACK_BUILD=1001 -D_DEFAULT_SOURCE -DVERSION_STR="0.0.0" \
	-std=c99
LDFLAGS := -lpthread

# Build targets
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

build: $(BUILD_DIR)
	$(HOST_CC) $(CFLAGS) $(INCLUDES) $(SOURCES) -o $(BINARY) $(LDFLAGS)
	@echo "Stub binary built: $(BINARY)"

run: build
	./$(BINARY)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf ../../stub_nvm_data

.PHONY: help build run test clean