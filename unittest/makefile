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
CMOCK_ROOT=./CMock

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


TARGET_BASE1=test_1
TARGET1 = $(TARGET_BASE1)$(TARGET_EXTENSION)
SRC_FILES1=$(UNITY_ROOT)/src/unity.c $(SOURCES) test/test_1.c  test/test_runners/test_1_runner.c
INC_DIRS=-I$(UNITY_ROOT)/src -I$(SRC_DIR) -I$(SRC_DIR)/hal -I$(CMOCK_ROOT)/src -I./mocks
SYMBOLS=

all: setup clean default

default: $(SRC_FILES1)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_FILES1) -o $(TARGET1)
	- ./$(TARGET1)
	
test/test_runners/test_1_runner.c: test/test_1.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb test/test_1.c test/test_runners/test_1_runner.c

../test_tools:
	mkdir -p ../test_tools

../test_tools/unity: | ../test_tools
	mkdir -p ../test_tools/unity

../test_tools/unity/download.zip: | ../test_tools/unity
	@echo "Downloading Unity"
	curl -L "https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v2.6.1.zip" \
	  -o ../test_tools/unity/download.zip \
		--fail --show-error --connect-timeout 30

../test_tools/unity/temp: | ../test_tools/unity/download.zip
	@echo "Extracting Unity"
	@unzip -q ../test_tools/unity/download.zip -d ../test_tools/unity/temp

../test_tools/unity/src: | ../test_tools/unity/temp
	@echo "Copying Src Unity Files"
	mv ../test_tools/unity/temp/*/* ../test_tools/unity

setup: | ../test_tools/unity/src

clean:
	$(CLEANUP) $(TARGET1)

ci: CFLAGS += -Werror
ci: default