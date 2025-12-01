*Open the **Outline** (table of contents) from the top right.*  

# Updating

This page describes updating supported devices OTA (both converting from original firmware and version bumps).  

> [!CAUTION]  
> OTA flashing and updates from the main branch are generally safe.  
> However, **always check [changelog_fw.md](./changelog_fw.md) and [known_issues.md](./known_issues.md) before updating!**  
> 
> Misuse of advanced options, bugs in the code and ignoring instructions **can brick your device**.  
> The only way to recover it is [contribute/flashing_via_wire.md](/docs/contribute/flashing_via_wire.md).  
> The same method is used for restoring the original FW, additionally requiring a memory dump of the stock device.

To receive custom FW updates, your ZHA / Z2M instance must have a **custom OTA index** applied.  
Additionally, to use the new features, you must also **download and regularly update the quirks / converters**.  

> [!TIP]  
> **Zigbee2MQTT is recommended** and prioritized by the contributors.  
> **ZHA is missing the interview button**, so devices might require re-pairing after version updates (to support new features).
>  
> Consider yourself invited to our [Discord](/readme.md#discord) community! 

## First-time update steps

**Original fw ➡ custom fw:**

1. Find your device on [supported_devices.md](./supported_devices.md) (Zigbee Manufacturer)
2. Read [known_issues.md](./known_issues.md)
3. Download the custom [# Quirks / Converters](#quirks--converters)
4. Apply the preferred [# OTA index](#ota-index) (not FORCE)
5. Restart ZHA / Z2M
6. Bring the device closer to the coordinator, or add routers to boost signal
(Pairing requires stronger signal than normal usage)
7. Optionally, tweak Z2M settings for [# Faster OTA updates](#faster-ota-updates)
8. Perform the OTA update (can get stuck at 100%, it's ok)
9. **Permit join** when it's done (the LED flashes if mapped correctly)
10. **Interview** the device **`i`**  
⤷ option missing from ZHA, remove and re-pair if needed  
11. **Reconfigure** the device **`🗘`** 
12. Be aware of [## Version update steps](#version-update-steps)
13. Read [faq.md](./faq.md) before reaching out for support

**If your device joins as the old entry**:  
⤷ Interview and Reconfigure it!  

**If your device joins as a new entry**:  
⤷ Force-remove the old & unresponsive entry.  

> Hopefully, you now have a working device with custom firmware! 😊  

## Version update steps

**Custom fw ➡ custom fw:**

1. Check if you have the correct index
2. Read [changelog_fw.md](./changelog_fw.md) and [known_issues.md](./known_issues.md)
3. Write down your configurations and binds  
(Some updates erase the user settings - mentioned in step 2)
4. Bring the device closer to the coordinator, or add routers to boost signal
(Pairing requires stronger signal than normal usage)
5. Optionally, tweak Z2M settings for [# Faster OTA updates](#faster-ota-updates)
6. Perform the OTA update
7. Redownload the custom [# Quirks / Converters](#quirks--converters) and restart ZHA / Z2M
8. **Interview** the device **`i`**  
⤷ option missing from ZHA, remove and re-pair instead  
(updates endpoints, clusters and identifiers)
9. **Reconfigure** the device **`🗘`**  
(resets reporting and stuff?, keeps user binds and settings)
10. Re-do user settings if needed

> [!NOTE]  
> If your device is several versions behind, it will update directly to the latest version.

# OTA index

The index is a list of links to firmware images.  
We use a link to the latest custom index so it's always up to date.  

### Changing the index

The custom OTA index must be written in one of the following places: 

<details>
<summary> Z2M user interface</summary>  

Z2M ➡ Settings ➡ OTA updates ➡ OTA index override file name

</details>

<details>
<summary> <code>zigbee2mqtt/configuration.yaml</code> or <code>data/configuration.yaml</code> for Z2M </summary>  

_The path differ depending on version/installation method. The file must already exist._  

For current Z2M version:
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

_The path might differ depending on version/installation method. The file must already exist._  
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

More details about ZHA updating here: [#62][zha_tips]
</details>
<br>

**A restart is required** (HA or Z2M) for the new index to apply.

### Choosing an index

You have to choose between **Router and EndDevice** operation modes before installing the firmware.  
Switching between them requires reinstalling (updating again with the FORCE index). 

- Router is recommended for L+N devices.  
- EndDevice is recommended for L-only devices.

Routing takes more power. It can be unstable for L-only devices and will require a higher load. Proceed with caution!  

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
> The custom OTA index appends to the original Z2M index.  
> So your other Zigbee devices will still receive updates normally.

### Faster OTA updates

**Zigbee is very low bandwidth**.  
By default, updates perform slowly to put less strain on the network and ensure stability.  
This may take hours, so if your device has good signal and the network is not being actively used, you can lower the duration to a few minutes.  

**Go to Z2M ➡ Settings ➡ OTA updates and tweak the values.**  
50B + 50ms takes less than 3 minutes on an empty network.

Read the [official Z2M docs][z2m_ota_speed] for more information.

# Quirks / Converters

Quirks and converters work like a database.  
They are responsible for recognizing devices and exposing all the supported features + showing correct name and picture.  

### Official quirks / converters (for stock firmware)

Manufacturers rarely guarantee ZHA/Z2M support, meaning **most of the converters are user contributions**.  
We can enjoy a universal smart home today just because users reported their findings and developers took the time to implement them.  

Before you install custom firmware, check the exposed features for your device.  
Recent devices should have: Correct name and picture, Power-on behavior, Switch type, LED control, possibly Countdown, Child lock.

If your device is missing some of those (and you think it supports them) consider contributing to [zha-device-handlers] and [zigbee-herdsman-converters]. Even opening issues is helpful.  
– You won't need the original converters once you're on custom firmware, but this will help users using original firmware. Also, the names and pictures from the original converters will show in our custom converters.

### Custom quirks / converters (for custom firmware)

Installing custom firmware requires new converters. We hope to unify and integrate the custom converters with the official ones at some point.  
Until then, you have to **manually download and update them regularly**. Note that they are branch-specific.  

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
5. Reconfigure device (does not remove user settings or binds)

</details>
<br>

Updating the quirks / converters can:  
- support new features from FW updates
- support new devices
- fix or prevent issues
- improve ease-of-use & readability
- add warnings about newly found issues  

**Quirks / converters can independently be updated**, so you are advised to **regularly redownload them** even if there are no FW updates.  

Normally, quirks and converters are backwards-compatible.  
Meaning that – if you have devices on different versions, you can safely use the latest files.

> [!TIP]  
> Follow the **#announcements** channel to stay up-to-date: [readme.md # Discord](/readme.md#discord)


[quirks]: https://github.com/romasku/tuya-zigbee-switch/tree/main/zha
[converters]: https://github.com/romasku/tuya-zigbee-switch/tree/main/zigbee2mqtt/converters
[zha_tips]: https://github.com/romasku/tuya-zigbee-switch/issues/62
[zha-device-handlers]: https://github.com/zigpy/zha-device-handlers
[zigbee-herdsman-converters]: https://github.com/Koenkk/zigbee-herdsman-converters
[z2m_ota_speed]: https://www.zigbee2mqtt.io/guide/usage/ota_updates.html#advanced-configuration