
OUT_DIR += /proj/common /proj/drivers /proj/os

OBJS += \
$(BUILD_PATH)/proj/common/list.o \
$(BUILD_PATH)/proj/common/mempool.o \
$(BUILD_PATH)/proj/common/tlPrintf.o \
$(BUILD_PATH)/proj/common/string.o \
$(BUILD_PATH)/proj/common/utility.o \
$(BUILD_PATH)/proj/drivers/drv_gpio.o \
$(BUILD_PATH)/proj/drivers/drv_nv.o \
$(BUILD_PATH)/proj/drivers/drv_pm.o \
$(BUILD_PATH)/proj/drivers/drv_putchar.o \
$(BUILD_PATH)/proj/drivers/drv_pwm.o \
$(BUILD_PATH)/proj/drivers/drv_timer.o \
$(BUILD_PATH)/proj/drivers/drv_uart.o \
$(BUILD_PATH)/proj/drivers/drv_keyboard.o \
$(BUILD_PATH)/proj/drivers/drv_flash.o \
$(BUILD_PATH)/proj/drivers/drv_hw.o \
$(BUILD_PATH)/proj/drivers/drv_adc.o \
$(BUILD_PATH)/proj/os/ev.o \
$(BUILD_PATH)/proj/os/ev_buffer.o \
$(BUILD_PATH)/proj/os/ev_poll.o \
$(BUILD_PATH)/proj/os/ev_queue.o \
$(BUILD_PATH)/proj/os/ev_timer.o \
$(BUILD_PATH)/proj/os/ev_rtc.o \
$(BUILD_PATH)/proj/drivers/drv_calibration.o \
$(BUILD_PATH)/proj/drivers/drv_security.o


#$(BUILD_PATH)/proj/os/ev_rtc.o \
#$(BUILD_PATH)/proj/drivers/drv_spi.o \
#
#$(BUILD_PATH)/proj/drivers/drv_i2c.o \
#$(BUILD_PATH)/proj/drivers/drv_keyboard.o \
#$(BUILD_PATH)/proj/drivers/drv_hw.o \
#$(BUILD_PATH)/proj/drivers/drv_flash.o \

# Each subdirectory must supply rules for building sources it contributes
$(BUILD_PATH)/proj/%.o: $(SDK_PATH)/proj/%.c
	@echo 'Building file: $<'
	@$(CC) $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"
