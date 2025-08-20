# 🛠️ Building

Buliding consists of multiple steps:
1. (one-time) installing dependencies
2. (one-time) installing SDK and toolchain
3. (optional) clearing OTA index files
4. cleaning previous firmware build (for every board)
5. building updated firmware (for every board)
6. generating new index files
7. updating Z2M converters (for old and new Z2M versions)
8. updating ZHA quirks
9. updating [supported_devices.md](./supported_devices.md)
10. (manual) updating [changelog_fw.md](./changelog_fw.md)
11. running unit tests (automated on push and merge)
11. (online) freezing OTA links  

The process is automated with scripts that you can run locally or online.  
  
> [!IMPORTANT]  
> We currently generate a dedicated firmware binary **for each device**.  
> The only difference between binaries is the **pre-defined config string** (device name and pinout).  
> We are slowly moving towards a unique build **(with an empty config string)** where the user will have to select the appropriate config string.  

Below are explicit instructions for building, installing and contributing.  

## ☁️ Online build (GitHub Actions)

Two branches are recommended to avoid conflicts between generated files.  
(Skip if you don't plan to merge.)

1. Fork the repository and clone it
2. Create **code_branch** from main (eg. newFeature)
3. Make changes
4. Update `changelog.md`
5. Commit changes and push
6. Create **build_branch** from **code_branch** (newFeature -> newFeature_build) and push
7. Visit GitHub Actions on your fork (web) and run `build.yml` on **build_branch**  
(this takes 5 minutes as it builds the firmware for every device)
8. Add the updated converters/quriks to your Z2M/ZHA instance  
(if new ones were generated)
9. Prepare the update
    - For wireless update, use the corresponding index in your OTA settings  
    (user + **build_branch** + device_type) 
    - For wire update, get the binary file for your device  
    (`bin/BOARD/tlc_switch.bin`)
10. Perform device update and test: [readme.md # Flashing](../readme.md#-flashing)
11. Create a Pull Request (**code_branch** -> **romasku/main**)
12. Check the unit tests result

## 💻 Local build

This project uses:
- **Make** for building, with all rules defined in Makefile
- **Python** for helper_scripts and ZHA quirks
- **Javascript** for Z2M converters  
- **YAML** for the device database

**Linux is recommended.**  
We currently have bash scripts for Debian/Ubuntu to install dependencies with apt and automate building for multipe boards.  
They can easily be adapted for other distributions. (Please share your scripts)

1. Fork the repository and clone it
2. Run `make_scripts/make_install.sh`
3. Create **code_branch** from main (eg. newFeature)
4. Make changes
5. Build with `make_scripts/make_all.sh` or `make_scripts/make_debug_single.sh`
6. Run unit tests with `make tests`: [tests.md](./tests.md)
7. Perform device update and test: [flashing_via_wire.md](./flashing_via_wire.md)
8. Update `changelog.md`
9. Commit changes (without generated files) and push
10. Create a Pull Request (**code_branch** -> **romasku/main**)

### Available commands

| Command                            | Description                                     |
|------------------------------------|-------------------------------------------------|
|`make_scripts/make_install.sh`      | (Re)installs dependencies (apt)                 |
|`make_scripts/make_all.sh`          | (Re)builds for all devices: <br> firmware, index, converters, quirks, supported devices list |
|`make_scripts/make_debug_single.sh` | (Re)builds for a single device, UART prints enabled: <br> firmware, index, converters, quirks, supported devices list |
|`make update_converters`            | (Re)generates Z2M converters                    |
|`make update_zha_quirk`             | (Re)generates ZHA quirks                        |
|`make update_supported_devices`     | (Re)generates [supported_devices.md](./supported_devices.md) |
|`make freeze_ota_links`             | Replaces branch with commit ID in index files. <br> Without it, old indexes will point to the latest fw |
|`make clean_z2m_index`              | Deletes OTA indexes                             |
|`make`                              | Builds fw and indexes for a single device       |
|`make clean`                        | Deletes the built fw binary for a single device |
|`make tests`                        | Runs the unit [tests.md](./tests.md)            |
|`make install`                      | `make sdk` and `make toolchain`                 |
|`make clean_install`                | Runs `make clean_sdk` and `make clean_toolchain`|
|`make sdk`                          | Downloads Telink's Zigbee SDK into `sdk/`       |
|`make clean_sdk`                    | Deletes `sdk/`                                  |
|`make toolchain`                    | Downloads Telink's tc32 into `toolchain/`       |
|`make clean_toolchain`              | Deletes `toolchain/`                            |
