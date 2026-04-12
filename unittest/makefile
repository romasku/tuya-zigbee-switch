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

PROJECT_ROOT=..
UNITY_ROOT=$(PROJECT_ROOT)/test_tools/unity
CMOCK_ROOT=$(PROJECT_ROOT)/test_tools/cmock

CFLAGS := -Wall -Wno-unused-parameter -Wno-unused-variable -g -O0 \
    -DHAL_STUB -DSTACK_BUILD=1001 -D_DEFAULT_SOURCE -DVERSION_STR="0.0.0" \
	-DNVM_MIGRATIONS_VERSION=1 \
	-std=c99

# Source files for stub build
SRC_DIR            := ../src
SOURCES := \
	$(CMOCK_ROOT)/src/cmock.c \
	./mocks/Mockgpio.c \
	./mocks/gpio_callback_helper.c \
	./mocks/Mocktimer.c \
	$(SRC_DIR)/base_components/encoder.c \


TARGET_BASE1=test_encoder
TARGET1 = $(TARGET_BASE1)$(TARGET_EXTENSION)
SRC_FILES1=$(UNITY_ROOT)/src/unity.c $(SOURCES) test/test_encoder.c  test/test_runners/test_encoder_runner.c
INC_DIRS=-I$(UNITY_ROOT)/src -I$(SRC_DIR) -I$(SRC_DIR)/hal -I$(CMOCK_ROOT)/src -I./mocks
SYMBOLS=

unity/%:
	$(MAKE) -f unity.mk $*

cmock/%:
	$(MAKE) -f cmock.mk $*

all: unity/setup cmock/setup clean default

default: $(SRC_FILES1)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_FILES1) -o $(TARGET1)
	- ./$(TARGET1)
	
test/test_runners/test_encoder_runner.c: test/test_encoder.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb test/test_encoder.c test/test_runners/test_encoder_runner.c

clean:
	$(CLEANUP) $(TARGET1)

ci: CFLAGS += -Werror
ci: default