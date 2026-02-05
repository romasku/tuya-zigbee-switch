*Open the **Outline** (table of contents) from the top right.*  

# Updating OTA

This page describes **converting** and **updating** supported devices **wirelessly.**

- ***Zigbee2MQTT is heavily recommended***
- *ZHA can't re-interview and update pinouts*
- *HOMEd supports only local updates and can't edit pinouts*

> [!CAUTION]  
> The main branch is generally safe. However, bugs in the code **can brick your device**.  
> - Recover or restore: [flashing/](./flashing/)  
> - Restoring the original FW requires a memory dump of the stock device: [`bin/_factory/`](../bin/_factory/)

- To receive custom FW updates, your ZHA / Z2M instance must have a **custom OTA index** applied.  
- To use the new features, you must **download and regularly update the quirks / converters / extensions**.  

## Conversion

**Original fw ➡ custom fw** update steps (only Telink devices):  

1. Find your device on [supported_devices.md](./supported_devices.md)
2. Read [known_issues.md](./known_issues.md)
3. **Download** the custom [# Quirks / Converters / Extensions](#quirks--converters--extensions)
4. **Apply** the preferred [# OTA index](#ota-index) (not FORCE)
5. **Restart** HA / Z2M
6. Bring the device closer to the coordinator, or add routers to **boost signal**  
  (Pairing requires stronger signal than normal usage)
7. Ensure **stable power** for no-Neutral switches (big load - non-smart bulb)
8. Optionally, tweak Z2M settings for [# Faster OTA updates](#faster-ota-updates)
9. **Check** for updates (blue cloud button) 
10. **Start** the update (red download button)  
  Progress bar may get stuck at 100% - it's ok
11. **Permit join** when it's done (LED blinks if mapped correctly)
12. **Interview** the device **`i`**  
  ⤷ option missing from ZHA, remove and re-pair if needed  
13. **Reconfigure** the device **`🗘`** 
14. Be aware of [## Version update](#version-update) steps
15. Read [faq.md](./faq.md) before reaching out for support

> Hopefully, you now have a working device with custom firmware! 😊  
> *Consider yourself invited to our [Discord](/readme.md#discord) community!* 

## Version update

**Custom fw ➡ custom fw** update steps (all devices):

1. Check if you have the correct [# OTA index](#ota-index)
2. Read [changelog_fw.md](./changelog_fw.md)
3. Remember your configurations and binds  
  (In case update erases them - if mentioned in step 2)
4. Ensure **strong signal**  
5. Ensure **stable power** for no-Neutral switches (big load - non-smart bulb)
6. Optionally, tweak Z2M settings for [# Faster OTA updates](#faster-ota-updates)
7. **Check** for updates (blue cloud button) 
8. **Start** the update (red download button) 
9. **Re-download** the custom [# Quirks / Converters / Extensions](#quirks--converters--extensions) and restart ZHA / Z2M
10. **Interview** the device **`i`**  
⤷ option missing from ZHA, remove and re-pair if needed  
(updates endpoints, clusters and identifiers)
11.  **Reconfigure** the device **`🗘`**  
(resets reporting and stuff?, keeps user binds and settings)
12.   Re-do user settings if needed

> *If your device is several versions behind, it will update directly to the latest version.*

# OTA index

The index points to the firmware images.  
We use a link to the latest index (so it's always up-to-date).  

### Changing the index

<details>
<summary> Z2M</summary>  

Apply custom index in the user interface and **restart Z2M:**  
**Z2M ➡ Settings ➡ OTA updates ➡ OTA index override file name**  

Or in `configuration.yaml`:
```yaml
ota:
  zigbee_ota_override_index_location: >-
    LINK_OR_PATH
```

</details>

<details>
<summary> ZHA </summary>  

Edit `homeassistant/configuration.yaml` and **restart HA**:  
(More details: [#62][zha_tips])

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

### Choosing an index

- You can install **Router** or **EndDevice** firmware
- Switch between them by re-installing (update again with FORCE index)
- **Router** can be more responsive
- **EndDevice** is useful for no-Neutral switches, because it draws less power (still more than original fw).  
  It's also useful when you don't want the device to route Zigbee traffic
  (e.g. If it's often powered-off, or if it has poor signal)

> [!WARNING]
> **L-only switches need a big load to function, on any firmware**
> - e.g. 10W dumb bulb is safe 
> - estimated values: >4W for EndDevice, >8W for Router  
> - **not recommended: no-Neutral switch + smart bulb ( <1W when brightness=0 )**  
>   ⤷ dummy load (capactior) may be required

<details>
<summary> <b> Index link format </b> </summary>  

```
https://raw.githubusercontent.com/USER/REPO/refs/heads/BRANCH/zigbee2mqtt/ota/INDEX
```
</details>
<br>

The available indexes **for the main branch** are:  

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
- Allows (re-installing FW with the same version number
- Only custom FW devices receive updates
- Useful for switching between operation modes or branches
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
- Allows re-installing FW with the same version number
- Only custom FW devices receive updates
- Useful for switching between operation modes or branches
```
https://raw.githubusercontent.com/romasku/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_end_device-FORCE.json
```
</details>
<br>

> [!NOTE]  
> The custom OTA index appends to the original Z2M index.  
> So your other Zigbee devices will still receive updates normally.

### Faster OTA updates

By default, updates perform slowly to put less strain on the network and ensure stability.  
If your device has good signal and the network is not being actively used, you can lower the duration:

Go to **Z2M ➡ Settings ➡ OTA updates** and tweak the values. More info: [Z2M doc][z2m_ota_speed].  
*50B + 50ms takes less than 5 minutes on an empty network*  
*(default_maximum_data_size and image_block_response_delay).*

# Quirks / Converters / Extensions

Custom quirks / converters / extensions:

- are needed for recognizing the device and exposing all supported features 

- can be updated independently of fw updates (e.g. support new devices)

- are generally backwards-compatible (Use the latest files)

- are not yet submitted to [ZHA][zha-device-handlers], [Z2M][zigbee-herdsman-converters] and [HOMEd][homed-service-zigbee]  
**⤷ manually download and update them**

### Download

<details>
<summary> ZHA steps </summary>  

1. (Re)download them from [`zha/`][quirks] (main branch)  
2. (Re)place them in `homeassistant/custom_zha_quirks/`  
3. Specify the custom quirks path in the configuration file (see [# OTA index](#ota-index))
4. Restart HA to apply
5. Reconfigure device (does not remove user settings or binds)

</details>

<details>
<summary> Z2M steps </summary>  

1. (Re)download them from [`zigbee2mqtt/converters/`][converters] (main branch)  
2. Create the `external_converters/` folder in `zigbee2mqtt/` or `data/`  
_The path differs depending on version/installation method. The parent folder must already exist._  
The new folder should be at the same level with coordinator_backup.json.  
3. (Re)place them in `external_converters/`
4. Restart Z2M to apply
5. Interview + reconfigure device (does not remove user settings or binds)

</details>

<details>
<summary> HOMEd steps </summary>  

1. (Re)download them from [`homed/`][extensions] (main branch)
2. (Re)place them in `/opt/homed-zigbee/external`  
_Or your custom path if you changed it in `homed-zigbee.conf` file (`device/external` setting)._
3. Restart HOMEd Zigbee to apply
4. Reconfigure device (does not remove user settings or binds) or remove it and add again

</details>

[quirks]: https://github.com/romasku/tuya-zigbee-switch/tree/main/zha
[converters]: https://github.com/romasku/tuya-zigbee-switch/tree/main/zigbee2mqtt/converters
[extensions]: https://github.com/romasku/tuya-zigbee-switch/tree/main/homed
[zha_tips]: https://github.com/romasku/tuya-zigbee-switch/issues/62
[zha-device-handlers]: https://github.com/zigpy/zha-device-handlers
[zigbee-herdsman-converters]: https://github.com/Koenkk/zigbee-herdsman-converters
[z2m_ota_speed]: https://www.zigbee2mqtt.io/guide/usage/ota_updates.html#advanced-configuration
[homed-service-zigbee]: https://github.com/u236/homed-service-zigbee