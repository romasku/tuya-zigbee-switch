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
9. updating `supported_devices.md`
10. (manual) updating `changelog.md`
11. (online) freezing OTA links  

The process is automated with scripts that you can run locally or online.  
  
### Note
> We currently generate a dedicated firmware binary **for each device**.  
The only difference between binaries is the **pre-defined config string** (device name and pinout).  
We are slowly moving towards a unique build **(with an empty config string)** where the user will have to select the appropriate config string.

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
(this takes 5 minutes as it builds the firmware for evey device)
8. Add the updated converters/quriks to your Z2M/ZHA instance  
(if new ones were generated)
9. Prepare the update
    - For wireless update, use the corresponding index in your OTA settings  
    (user + **build_branch** + device_type) 
    - For wire update, get the binary file for your device  
    (`bin/BOARD/tlc_switch.bin`)
10. Perform update and test: [readme.md # Flashing](../readme.md#-flashing)
11. Create a Pull Request (**code_branch** -> **romasku/main**)

## 💻 Local build

Linux is recommended.  
We currently have scripts for Debian/Ubuntu, but they can easily be adapted for other distributions.  
(Please share your scripts)

1. Fork the repository and clone it
2. Run `make_scripts/make_install.sh`
3. Create **code_branch** from main (eg. newFeature)
4. Make changes
5. Build with `make_scripts/make_all.sh` or `make_scripts/make_debug_single.sh`
6. Perform update and test: [flashing_via_wire.md](../flashing_via_wire.md)
7. Update `changelog.md`
8. Commit changes (without generated files) and push
9. Create a Pull Request (**code_branch** -> **romasku/main**)

### Available commands

- (Re)install dependencies:  

`make_scripts/make_install.sh`

- (Re)build for all devices - firmware, index, converters, quirks, supported devices list: 

`make_scripts/make_all.sh`

- (Re)build for a single device, UART prints enabled - firmware, index, converters, quirks, supported devices list:  

`make_scripts/make_debug_single.sh`

- (Re)generate Z2M converters:  

`make update_converters`

- (Re)generate ZHA quirks:  

`make update_zha_quirk`

- (Re)generate supported devices list:  

`make update_supported_devices`