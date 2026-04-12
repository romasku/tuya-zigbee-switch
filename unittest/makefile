# =========================================================================
#   Unity - A Test Framework for C
#   ThrowTheSwitch.org
#   Copyright (c) 2007-24 Mike Karlesky, Mark VanderVoord, & Greg Williams
#   SPDX-License-Identifier: MIT
# =========================================================================

#We try to detect the OS we are running on, and adjust commands as needed
ifeq ($(OS),Windows_NT)
  ifeq ($(shell uname -s),) # not in a bash-like shell
	CLEANUP = del /F /Q
	MKDIR = mkdir
  else # in a bash-like shell, like msys
	CLEANUP = rm -f
	MKDIR = mkdir -p
  endif
	TARGET_EXTENSION=.exe
else
	CLEANUP = rm -f
	MKDIR = mkdir -p
	TARGET_EXTENSION=.out
endif

C_COMPILER=gcc
ifeq ($(shell uname -s), Darwin)
C_COMPILER=clang
endif

UNITY_ROOT=./unity

CFLAGS=-std=c89
CFLAGS += -Wall
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-unused-variable
# CFLAGS += -Wextra
# CFLAGS += -Wpointer-arith
# CFLAGS += -Wcast-align
# CFLAGS += -Wwrite-strings
# CFLAGS += -Wunreachable-code
# CFLAGS += -Winit-self
# CFLAGS += -Wmissing-field-initializers
# CFLAGS += -Wno-unknown-pragmas
# CFLAGS += -Wundef
# CFLAGS += -fgnu89-inline
#CFLAGS += -Wno-misleading-indentation
CFLAGS += -Wall -Wno-unused-parameter -Wno-unused-variable -g -O0 \
    -DHAL_STUB -DSTACK_BUILD=1001 -D_DEFAULT_SOURCE -DVERSION_STR="0.0.0" \
	-DNVM_MIGRATIONS_VERSION=1 \
	-std=c99

# Source files for stub build
SRC_DIR            := ../src
SOURCES := \
	$(SRC_DIR)/zigbee/poll_control_cluster.c \
	$(SRC_DIR)/stub/hal/gpio.c \
	$(SRC_DIR)/stub/hal/system.c \
	$(SRC_DIR)/stub/hal/timer.c \
	$(SRC_DIR)/stub/hal/tasks.c \
	$(SRC_DIR)/stub/hal/nvm.c \
	$(SRC_DIR)/stub/hal/adc.c \
	$(SRC_DIR)/stub/hal/zigbee.c \
	$(SRC_DIR)/stub/hal/ota.c \
	$(SRC_DIR)/stub/machine_io.c \
	$(SRC_DIR)/base_components/led.c \
	$(SRC_DIR)/base_components/relay.c \
	$(SRC_DIR)/base_components/button.c \
	$(SRC_DIR)/base_components/battery.c \
	$(SRC_DIR)/base_components/network_indicator.c \
	$(SRC_DIR)/base_components/encoder.c \
	$(SRC_DIR)/device_config/config_parser.c \
	$(SRC_DIR)/device_config/config_nv.c \
	$(SRC_DIR)/device_config/device_params_nv.c \
	$(SRC_DIR)/device_config/reset.c \
	$(SRC_DIR)/device_config/nvm_migrations.c \
	$(SRC_DIR)/zigbee/basic_cluster.c \
	$(SRC_DIR)/zigbee/relay_cluster.c \
	$(SRC_DIR)/zigbee/switch_cluster.c \
	$(SRC_DIR)/zigbee/group_cluster.c \
	$(SRC_DIR)/zigbee/general_commands.c \
	$(SRC_DIR)/zigbee/cover_switch_cluster.c \
	$(SRC_DIR)/zigbee/cover_cluster.c \
	$(SRC_DIR)/zigbee/battery_cluster.c

TARGET_BASE1=test_1
TARGET1 = $(TARGET_BASE1)$(TARGET_EXTENSION)
SRC_FILES1=$(UNITY_ROOT)/src/unity.c $(SOURCES) test/test_1.c  test/test_runners/test_1_runner.c
INC_DIRS=-I$(UNITY_ROOT)/src -I$(SRC_DIR) 
SYMBOLS=

all: clean default

default: $(SRC_FILES1)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_FILES1) -o $(TARGET1)
	- ./$(TARGET1)
	
test/test_runners/test_1_runner.c: test/test_1.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb test/test_1.c test/test_runners/test_1_runner.c

clean:
	$(CLEANUP) $(TARGET1)

ci: CFLAGS += -Werror
ci: default