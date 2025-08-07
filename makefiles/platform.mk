
OUT_DIR += /platform \
/platform/boot \
/platform/boot/8258 \
/platform/services/b85m \
/platform/tc32 \
/platform/chip_8258 \
/platform/chip_8258/flash

OBJS += \
$(BUILD_PATH)/platform/boot/link_cfg.o \
$(BUILD_PATH)/platform/services/b85m/irq_handler.o \
$(BUILD_PATH)/platform/tc32/div_mod.o \
$(BUILD_PATH)/platform/chip_8258/flash.o \
$(BUILD_PATH)/platform/boot/8258/cstartup_8258.o \
$(BUILD_PATH)/platform/chip_8258/adc.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_common.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid1060c8.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid1360c8.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid011460c8.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid134051.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid136085.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid1360eb.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid14325e.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid1460c8.o \
$(BUILD_PATH)/platform/chip_8258/flash/flash_mid13325e.o


#$(BUILD_PATH)/platform/chip_8258/flash/flash_mid136085.o \

# Each subdirectory must supply rules for building sources it contributes
$(BUILD_PATH)/platform/%.o: $(SDK_PATH)/platform/%.c
	@echo 'Building file: $<'
	@$(CC) $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"

$(BUILD_PATH)/platform/%.o: $(SDK_PATH)/platform/%.S
	@echo 'Building file: $<'
	@$(CC) $(GCC_FLAGS) $(ASM_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"
