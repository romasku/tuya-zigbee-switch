# Telink SDK Sources Makefile
# This makefile handles compilation of all Telink SDK sources

# SDK paths
SDK_PATH := $(TELINK_TOOLS_DIR)/sdk

PLATFORM_SOURCES := \
	$(SDK_PATH)/platform/boot/8258/cstartup_8258.S \
    $(SDK_PATH)/platform/boot/link_cfg.S \
	$(SDK_PATH)/platform/services/b85m/irq_handler.c \
	$(SDK_PATH)/platform/tc32/div_mod.S \
	$(SDK_PATH)/platform/chip_8258/flash.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_common.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid1060c8.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid1360c8.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid011460c8.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid134051.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid136085.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid1360eb.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid14325e.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid1460c8.c \
	$(SDK_PATH)/platform/chip_8258/flash/flash_mid13325e.c \
	$(SDK_PATH)/platform/chip_8258/adc.c

PROJ_SOURCES := \
	$(SDK_PATH)/proj/common/list.c \
	$(SDK_PATH)/proj/common/mempool.c \
	$(SDK_PATH)/proj/common/tlPrintf.c \
	$(SDK_PATH)/proj/common/string.c \
	$(SDK_PATH)/proj/common/utility.c \
	$(SDK_PATH)/proj/drivers/drv_gpio.c \
	$(SDK_PATH)/proj/drivers/drv_adc.c \
	$(SDK_PATH)/proj/drivers/drv_nv.c \
	$(SDK_PATH)/proj/drivers/drv_pm.c \
	$(SDK_PATH)/proj/drivers/drv_putchar.c \
	$(SDK_PATH)/proj/drivers/drv_timer.c \
	$(SDK_PATH)/proj/drivers/drv_uart.c \
	$(SDK_PATH)/proj/drivers/drv_calibration.c \
	$(SDK_PATH)/proj/drivers/drv_flash.c \
	$(SDK_PATH)/proj/drivers/drv_hw.c \
	$(SDK_PATH)/proj/drivers/drv_security.c \
	$(SDK_PATH)/proj/os/ev.c \
	$(SDK_PATH)/proj/os/ev_buffer.c \
	$(SDK_PATH)/proj/os/ev_poll.c \
	$(SDK_PATH)/proj/os/ev_queue.c \
	$(SDK_PATH)/proj/os/ev_timer.c \
	$(SDK_PATH)/proj/os/ev_rtc.c

ZIGBEE_SOURCES := \
	$(SDK_PATH)/zigbee/bdb/bdb.c \
	$(SDK_PATH)/zigbee/aps/aps_group.c \
	$(SDK_PATH)/zigbee/mac/mac_phy.c \
	$(SDK_PATH)/zigbee/mac/mac_pib.c \
	$(SDK_PATH)/zigbee/zdo/zdp.c \
	$(SDK_PATH)/zigbee/zcl/zcl.c \
	$(SDK_PATH)/zigbee/zcl/zcl_nv.c \
	$(SDK_PATH)/zigbee/zcl/zcl_reporting.c \
	$(SDK_PATH)/zigbee/zcl/hvac/zcl_thermostat.c \
	$(SDK_PATH)/zigbee/zcl/smart_energy/zcl_metering.c \
	$(SDK_PATH)/zigbee/zcl/smart_energy/zcl_metering_attr.c \
	$(SDK_PATH)/zigbee/zcl/zll_commissioning/zcl_toucklink_security.c \
	$(SDK_PATH)/zigbee/zcl/zll_commissioning/zcl_zllTouchLinkDiscovery.c \
	$(SDK_PATH)/zigbee/zcl/zll_commissioning/zcl_zllTouchLinkJoinOrStart.c \
	$(SDK_PATH)/zigbee/zcl/zll_commissioning/zcl_zll_commissioning.c \
	$(SDK_PATH)/zigbee/zcl/commissioning/zcl_commissioning.c \
	$(SDK_PATH)/zigbee/zcl/commissioning/zcl_commissioning_attr.c \
	$(SDK_PATH)/zigbee/zcl/hvac/zcl_thermostat.c \
	$(SDK_PATH)/zigbee/zcl/measument_sensing/zcl_electrical_measurement.c \
	$(SDK_PATH)/zigbee/zcl/measument_sensing/zcl_electrical_measurement_attr.c \
	$(SDK_PATH)/zigbee/zcl/measument_sensing/zcl_illuminance_measurement.c \
	$(SDK_PATH)/zigbee/zcl/measument_sensing/zcl_illuminance_measurement_attr.c \
	$(SDK_PATH)/zigbee/zcl/measument_sensing/zcl_occupancy_sensing.c \
	$(SDK_PATH)/zigbee/zcl/measument_sensing/zcl_occupancy_sensing_attr.c \
	$(SDK_PATH)/zigbee/zcl/measument_sensing/zcl_temperature_measurement.c \
	$(SDK_PATH)/zigbee/zcl/measument_sensing/zcl_temperature_measurement_attr.c \
	$(SDK_PATH)/zigbee/zcl/light_color_control/zcl_light_colorCtrl.c \
	$(SDK_PATH)/zigbee/zcl/light_color_control/zcl_light_colorCtrl_attr.c \
	$(SDK_PATH)/zigbee/zcl/closures/zcl_door_lock.c \
	$(SDK_PATH)/zigbee/zcl/closures/zcl_door_lock_attr.c \
	$(SDK_PATH)/zigbee/zcl/closures/zcl_window_covering.c \
	$(SDK_PATH)/zigbee/zcl/closures/zcl_window_covering_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_alarm.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_alarm_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_basic.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_basic_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_binary_input.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_binary_input_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_binary_output.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_binary_output_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_devTemperatureCfg.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_devTemperatureCfg_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_diagnostics.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_diagnostics_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_greenPower.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_greenPower_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_group.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_group_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_identify.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_identify_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_level.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_level_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_multistate_input.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_multistate_input_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_multistate_output.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_multistate_output_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_onoff.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_onoff_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_pollCtrl.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_pollCtrl_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_powerCfg.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_powerCfg_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_scene.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_scene_attr.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_time.c \
	$(SDK_PATH)/zigbee/zcl/general/zcl_time_attr.c \
	$(SDK_PATH)/zigbee/zcl/security_safety/zcl_ias_ace.c \
	$(SDK_PATH)/zigbee/zcl/security_safety/zcl_ias_wd.c \
	$(SDK_PATH)/zigbee/zcl/security_safety/zcl_ias_wd_attr.c \
	$(SDK_PATH)/zigbee/zcl/security_safety/zcl_ias_zone.c \
	$(SDK_PATH)/zigbee/zcl/security_safety/zcl_ias_zone_attr.c \
	$(SDK_PATH)/zigbee/zcl/zcl_wwah/zcl_wwah.c \
	$(SDK_PATH)/zigbee/zcl/zcl_wwah/zcl_wwah_attr.c \
	$(SDK_PATH)/zigbee/zcl/ota_upgrading/zcl_ota.c \
	$(SDK_PATH)/zigbee/zcl/ota_upgrading/zcl_ota_attr.c \
	$(SDK_PATH)/zigbee/common/zb_config.c \
	$(SDK_PATH)/zigbee/af/zb_af.c \
	$(SDK_PATH)/zigbee/wwah/wwah.c \
	$(SDK_PATH)/zigbee/wwah/wwahEpCfg.c \
	$(SDK_PATH)/zigbee/gp/gp.c \
	$(SDK_PATH)/zigbee/gp/gpEpCfg.c \
	${SDK_PATH}/zigbee/gp/gp_proxy.c \
	$(SDK_PATH)/zigbee/gp/gp_proxyTab.c \
	$(SDK_PATH)/zigbee/ss/ss_nv.c \
	$(SDK_PATH)/zigbee/ota/ota.c \
	$(SDK_PATH)/zigbee/ota/otaEpCfg.c
ALL_SDK_SOURCES := $(PLATFORM_SOURCES) $(PROJ_SOURCES) $(ZIGBEE_SOURCES)

# Object files
PLATFORM_OBJS := $(PLATFORM_SOURCES:$(SDK_PATH)/%.c=$(BUILD_DIR)/sdk/%.o)
PLATFORM_OBJS := $(PLATFORM_OBJS:$(SDK_PATH)/%.S=$(BUILD_DIR)/sdk/%.o)
PROJ_OBJS := $(PROJ_SOURCES:$(SDK_PATH)/%.c=$(BUILD_DIR)/sdk/%.o)
ZIGBEE_OBJS := $(ZIGBEE_SOURCES:$(SDK_PATH)/%.c=$(BUILD_DIR)/sdk/%.o)

# All SDK object files
SDK_OBJS := $(ALL_SDK_SOURCES:$(SDK_PATH)/%.c=$(BUILD_DIR)/sdk/%.o)
SDK_OBJS := $(SDK_OBJS:$(SDK_PATH)/%.S=$(BUILD_DIR)/sdk/%.o)

# SDK dependency files (only for C files, not assembly)
SDK_C_SOURCES := $(filter %.c,$(ALL_SDK_SOURCES))
SDK_DEPS := $(SDK_C_SOURCES:$(SDK_PATH)/%.c=$(BUILD_DIR)/sdk/%.d)

# Build all SDK objects
sdk-build: $(SDK_OBJS)
	@echo "SDK compilation complete: $(words $(SDK_OBJS)) object files"

# Clean SDK build artifacts
sdk-clean:
	@echo "Cleaning SDK build artifacts..."
	@rm -rf $(BUILD_DIR)/sdk

# Include SDK dependency files (if they exist)
-include $(SDK_DEPS)

# Special compiler flags for specific SDK files
$(BUILD_DIR)/sdk/proj/drivers/drv_nv.o: GCC_FLAGS += -Dnv_resetToFactoryNew=nv_resetToFactoryNew__sdk

# Compile SDK C files
$(BUILD_DIR)/sdk/%.o: $(SDK_PATH)/%.c
	@echo "Compiling SDK $<"
	@mkdir -p $(dir $@)
	@$(CC) $(GCC_FLAGS) $(DEVICE_DEFS) $(INCLUDE_PATHS) -c -o $@ $<

# Compile SDK assembly files
$(BUILD_DIR)/sdk/%.o: $(SDK_PATH)/%.S
	@echo "Assembling SDK $<"
	@mkdir -p $(dir $@)
	@$(CC) $(ASM_FLAGS) $(DEVICE_DEFS) $(INCLUDE_PATHS) -c -o $@ $<

# Help for SDK makefile
sdk-help:
	@echo "Telink SDK Build System"
	@echo "======================="
	@echo ""
	@echo "Targets:"
	@echo "  sdk-build           - Build all SDK object files"
	@echo "  sdk-clean           - Clean SDK build artifacts"
	@echo "  sdk-directories     - Create SDK build directories"
	@echo "  sdk-help            - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  SDK_PATH            - Path to Telink SDK ($(SDK_PATH))"
	@echo "  BUILD_DIR           - Build output directory ($(BUILD_DIR))"
	@echo ""
	@echo "Source files:"
	@echo "  Platform sources:   $(words $(PLATFORM_SOURCES)) files"
	@echo "  Project sources:    $(words $(PROJ_SOURCES)) files"  
	@echo "  Zigbee sources:     $(words $(ZIGBEE_SOURCES)) files"
	@echo "  Total SDK sources:  $(words $(ALL_SDK_SOURCES)) files"

.PHONY: sdk-build sdk-clean sdk-directories sdk-help