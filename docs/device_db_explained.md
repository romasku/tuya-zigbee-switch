# Device database

Information about all supported devices is kept inside `device_db.yaml`.

> [!TIP]  
> That is the only file you need to edit for adding a new device  
> that has a supported Zigbee module (ZTU, ZT2S, ZT3L)

[off_conv]: https://github.com/Koenkk/zigbee-herdsman-converters/tree/master/src/devices

| Field                        | Description                                                                                                                  |
|------------------------------|------------------------------------------------------------------------------------------------------------------------------|
|**KEY** (e.g. `TS0004_AVATTO`)| **Given device name**: <br> • Used in project as `BOARD` <br> • Name of the device directory in `bin/`                       |
|`tuya_manufacturer_name`      | **Zigbee Manufacturer** in Z2M on stock FW: <br> • Identifies the exact device <br> • Used in OTA indexes to select the correct FW when updating <br> • Format: `_TZ3000_xxxxxxxx` |
|`stock_converter_model`       | **Model** in Z2M on stock FW: <br> • May be overwritten by `whiteLabel` <br> &nbsp; (Check [official Z2M converters](off_conv)) <br> • Needed to enable OTA for stock FW <br> • Used as base for the custom converter |
|`config_str`                  | Given `manufacturer;model;pinout` on custom FW: <br> • `manufacturer` → ID & OTA updates <br> • `model` → selects converter <br> • `pinout` → GPIO mapping |
|`alt_config_str`              | Alternative config string: <br> • In case the device has [multiple_pinouts.md](./multiple_pinouts.md) <br> • Currently unused|
|`device_type`                 | Default operation mode for custom FW: <br> • `end_device` → Line-only devices <br> • `router` → Line+Neutral devices <br> (Scripts build both variants for L-only devices) |
|`old_manufacturer_names`      | Previously used manufacturer names in custom FW                                                                              |
|`old_zb_models`               | Previously used model names in custom FW                                                                                     |
|`override_z2m_device`         | Overwrites the model name: <br> (get it from `whiteLabel` in [official Z2M converters](off_conv)): <br> • Keeps generic converter <br> • Adds custom name & picture <br> • eg. `TS0004_AVATTO` shows up as `ZWSM16-4-Zigbee`, <br> instead of generic `TS0004_switch_module_2`                                     |
|`tuya_image_type`             | Stock FW identifier: <br> • Needed for OTA updates <br> • Found in Z2M debug logs when attempting OTA                        |
|`firmware_image_type`         | Given custom FW identifier (usually 54179): <br> • Currently unique per device <br> • Increment for every new device         |
|`tuya_manufacturer_id`        | Another stock FW identifier (usually 4417): <br> • Needed for OTA updates <br> • Found in Z2M debug logs when attempting OTA |
|`zb_module`                   | Zigbee module inside the device: <br> • Currently unused, planned for pinout validation <br> • Supported: ZTU, ZT2S, ZT3L    |
|`human_name`                  | Full name of the device: <br> • Shown in [supported_devices.md](./supported_devices.md)                                      |
|`status`                      | Device support status: <br> • **Supported** or **In progress** <br> • Shown in [supported_devices.md](./supported_devices.md)|
|`github_issue`                | Link to device-related GitHub issue or pull request                                                                          |
|`store`                       | Link to buy the exact same device: <br> • Preferably AliExpress (international, English, no affiliation)                     |
