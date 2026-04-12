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
CMOCK_ROOT=./CMock

CFLAGS := -Wall -Wno-unused-parameter -Wno-unused-variable -g -O0 \
    -DHAL_STUB -DSTACK_BUILD=1001 -D_DEFAULT_SOURCE -DVERSION_STR="0.0.0" \
	-DNVM_MIGRATIONS_VERSION=1 \
	-std=c99

# Source files for stub build
SRC_DIR            := ../src
SOURCES := \
	$(CMOCK_ROOT)/src/cmock.c \
	$(CMOCK_ROOT)/lib/mocks/Mockgpio.c \
	$(CMOCK_ROOT)/lib/mocks/gpio_callback_helper.c \
	$(SRC_DIR)/stub/hal/timer.c \
	$(SRC_DIR)/stub/hal/tasks.c \
	$(SRC_DIR)/stub/machine_io.c \
	$(SRC_DIR)/base_components/encoder.c \


TARGET_BASE1=test_1
TARGET1 = $(TARGET_BASE1)$(TARGET_EXTENSION)
SRC_FILES1=$(UNITY_ROOT)/src/unity.c $(SOURCES) test/test_1.c  test/test_runners/test_1_runner.c
INC_DIRS=-I$(UNITY_ROOT)/src -I$(SRC_DIR) -I$(CMOCK_ROOT)/src -I$(CMOCK_ROOT)/lib/mocks
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