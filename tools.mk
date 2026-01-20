help:
	@echo "Tools Makefile - Integration and Documentation Commands"
	@echo ""
	@echo "Available targets:"
	@echo ""
	@echo "  Zigbee2MQTT:"
	@echo "    clean_z2m_index        - Clear all Zigbee2MQTT OTA index files"
	@echo "    update_converters      - Generate Z2M converter files based on device database"
	@echo "    freeze_ota_links       - Replace branch refs with commit hashes in OTA indices"
	@echo ""
	@echo "  ZHA:"
	@echo "    update_zha_quirk       - Generate ZHA quirks file from device database"
	@echo ""
	@echo "  Documentation:"
	@echo "    update_supported_devices - Generate supported devices documentation"
	@echo ""
	@echo "  Development Utilities:"
	@echo "    unused_image_type      - Show next available firmware image type ID"
	@echo ""


# Clear all Zigbee2MQTT index files
clean_z2m_index:
	@echo "[]" > zigbee2mqtt/ota/index_router-FORCE.json
	@echo "[]" > zigbee2mqtt/ota/index_end_device-FORCE.json
	@echo "[]" > zigbee2mqtt/ota/index_router.json
	@echo "[]" > zigbee2mqtt/ota/index_end_device.json

# Update Zigbee2MQTT converter files
update_converters:
	python3 helper_scripts/make_z2m_tuya_converters.py device_db.yaml \
		> zigbee2mqtt/converters/tuya_with_ota.js 
	python3 helper_scripts/make_z2m_tuya_converters.py --z2m-v1 device_db.yaml \
		> zigbee2mqtt/converters_v1/tuya_with_ota.js 
	python3 helper_scripts/make_z2m_custom_converters.py device_db.yaml \
		> zigbee2mqtt/converters/switch_custom.js 
	python3 helper_scripts/make_z2m_custom_converters.py --z2m-v1 device_db.yaml \
		> zigbee2mqtt/converters_v1/switch_custom.js 
	python3 helper_scripts/make_homed_extension.py device_db.yaml \
		> homed/tuya-custom.json 

update_zha_quirk:
	python3 helper_scripts/make_zha_quirk.py device_db.yaml > zha/switch_quirk.py


update_supported_devices:
	python3 helper_scripts/make_supported_devices.py device_db.yaml > docs/supported_devices.md 


freeze_ota_links:
	sed -i "s|refs\/heads\/$(shell git branch --show-current)|$(shell git rev-parse HEAD)|g" zigbee2mqtt/ota/*.json 


unused_image_type:
	@yq '[.[] | .firmware_image_type] | max + 1' device_db.yaml