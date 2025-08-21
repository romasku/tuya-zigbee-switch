[quirks]: https://github.com/romasku/tuya-zigbee-switch/tree/main/zha
[converters]: https://github.com/romasku/tuya-zigbee-switch/tree/main/zigbee2mqtt/converters

# Updating

> [!CAUTION]  
> OTA flashing and updates from the main branch are generally safe.  
> However, misuse of advanced options, bugs in the custom FW and ignoring instructions **can brick your device**.  
> The only way to recover it is [flashing_via_wire.md](./flashing_via_wire.md).  
> The same method is used for restoring to original FW, additionally requiring a memory dump of the stock device.

To receive custom FW updates, your ZHA / Z2M instance must have a **custom OTA index** applied.  
Additionally, to use the new features, you must also **download and regularly update the quirks / converters**.  

> [!TIP]  
> The contributors use, recommend and prioritize Z2M.

## OTA index

Changing the index can be done in Z2M settings or in: 

<details>
<summary> <code>zigbee2mqtt/data/configuration.yaml</code> for Z2M </summary>  

```yaml
ota:
  zigbee_ota_override_index_location: >-
    LINK_OR_PATH
```

Z2M versions older than v2.0.0 need a different configuration (and we will drop support soon):
```yaml
external_converters:
  - switch_custom.js
  - tuya_with_ota.js
ota:
  zigbee_ota_override_index_location: PATH
```
</details>

<details>
<summary> <code>homeassistant/configuration.yaml</code> for ZHA </summary>  
Note that we also enabled quirks.

```yaml
zha:
  enable_quirks: true
  custom_quirks_path: ./custom_zha_quirks/
  zigpy_config:
    ota:
      extra_providers:
        - type: z2m
          url: LINK_OR_PATH
```
</details>

**A restart is required** for the new index to apply.

You can choose between **Router and EndDevice** operation modes for your device.  
Switching between them requires updating with the FORCE index. 

Routing takes more power. It is fully safe for L+N devices,  
but it can be unstable for L-only devices and might require a higher load. Proceed with caution!  

The available indexes for the main branch are:  

<details>
<summary> <code> index_router.json </code> </summary>  

- Both L and L+N switches get Router FW
- Both stock and custom FW devices receive updates
```
https://raw.githubusercontent.com/romasku/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_router.json
```
</details>

<details>
<summary> <code> index_router-FORCE.json </code> </summary>  

- Both L and L+N switches get Router FW
- Allows (re)installing FW with the same version number
- Only custom FW devices receive updates
- Useful when developing, debugging, switching between operation modes
```
https://raw.githubusercontent.com/romasku/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_router-FORCE.json
```
</details>


<details>
<summary> <code> index_end_device.json </code> </summary>  

- L-only switches get EndDevice FW
- L+N switches do not get anything
- Both stock and custom FW devices receive updates
```
https://raw.githubusercontent.com/romasku/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_end_device.json
```
</details>


<details>
<summary> <code> index_end_device-FORCE.json </code> </summary>  

- L-only switches get EndDevice FW
- L+N switches do not get anything
- Allows (re)installing FW with the same version number
- Only custom FW devices receive updates
- Useful when developing, debugging, switching between operation modes
```
https://raw.githubusercontent.com/romasku/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_end_device-FORCE.json
```
</details>
<br>

> [!NOTE]  
> The custom OTA index actually appends to the original Z2M index.  
> So your other Zigbee devices will still receive updates normally.

## Quirks / Converters

Currently, you have to manually (re)download them from [`zha/`][quirks] or [`zigbee2mqtt/converters/`][converters]  
and (re)place them in `homeassistant/custom_zha_quirks/` or `zigbee2mqtt/data/external_converters` respectively.  

- The custom quirks path has to be specified in the configuration file (see OTA index).  
- The external converters directory has to be manually created.

The quirks / converters **independently receive updates** to:  
- support new features from FW updates
- support new devices
- fix or prevent issues
- improve ease-of-use & readability
- add warnings about newly found issues  

So you are advised to **regularly update the quirks / converters** even if there are no FW updates.  

Normally, quirks and converters are backwards-compatible.  
Meaning that if you have devices on different versions, you can safely use the latest files.

> [!TIP]  
> Follow the **#announcements** channel to stay up-to-date: [readme.md # Discord](../readme.md#-discord)

## First-time update steps
1. Download the custom quirks / converters
2. Apply the preferred OTA index
3. Restart ZHA / Z2M
4. Perform the OTA update

- If the device does not rejoin after the update:  
Force-remove the old device and open your network.  
If it's not already, put the device in pairing mode (blinking LED):  
Long-press the on-board button or quickly press any switch 5 times in a row.

- If the device rejoins: Interview and Reconfigure it.

Hopefully, you now have a working device with custom firmware! 😊  

## Version update steps
1. **Read the changelog**
2. Reset the device if mentioned in changelog  
(resetting erases the configuration from flash memory so it can prevent issues)
3. Perform the OTA update\*
4. Update the quirks / converters and restart ZHA / Z2M
5. Interview the device(s)  
(updates the manufacturer and model names)
6. Reconfigure the devices(s)  
(updates reporting and endpoints, keeps binds and user settings)

\*Alternatively, [flashing_via_wire.md](./flashing_via_wire.md) is possible: just un-tick *Erase memory* to update while keeping user settings.

> [!NOTE]  
> If your device is several versions behind, it will update directly to the latest verion.