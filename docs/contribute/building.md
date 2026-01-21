*Open the **Outline** (table of contents) from the top right.*  

# 🛠️ Building

Building consists of multiple steps:

1. (one-time) installing dependencies
2. (one-time) installing SDK and toolchain
3. (optional) clearing OTA index files
4. cleaning previous firmware build (for every board)
5. building updated firmware (for every board)
6. generating new index files
7. updating Z2M converters (for old and new Z2M versions)
8. updating ZHA quirks
9. updating [supported_devices.md](/docs/supported_devices.md)
10. (manual) updating [changelog_fw.md](/docs/changelog_fw.md)
11. running unit tests (automated on push and merge)
12. (online) freezing OTA links

The process is automated with scripts that you can run locally or online.

> [!IMPORTANT]  
> We currently generate a dedicated firmware binary **for each device**.  
> The only difference between binaries is the **pre-defined config string** (device name and pinout) + imageType.  
> We are slowly moving towards a unified build **(with an empty config string)** where the user will have to select the appropriate config string.

Below are explicit instructions for building, installing and contributing.

## ☁️ Online build (GitHub Actions)

Two branches are recommended to avoid conflicts between generated files.  
(Skip if you don't plan to merge.)

1. Fork the repository and clone it
2. Create **code_branch** from main (eg. newFeature)
3. Make changes
4. Update `changelog_fw.md` (manual)
5. Commit changes and push
6. Create **build_branch** from **code_branch** (newFeature ➡ newFeature_build) and push
7. Visit GitHub Actions on your fork (web) and run `build.yml` on **build_branch**  
   (this takes 5 minutes as it builds the firmware for every device)
8. Add the updated converters/quirks to your Z2M/ZHA instance  
   (if new ones were generated)
9. Prepare the update

   - For wireless update, use the corresponding index in your OTA settings  
     (user + **build_branch** + device_type)
   - For wire update, get the binary file for your device  
     - Telink: `bin/DEVICE_TYPE/BOARD/tlc_switch-X.Y.Z-<commit-hash>.bin`
     - Silabs: `bin/DEVICE_TYPE/BOARD/tlc_switch-X.Y.Z-<commit-hash>.s37`

10. Perform device update and test: [flashing/](../flashing/)
11. Create a Pull Request (**code_branch** ➡ **romasku/main**)
12. Check the unit tests result

## 💻 Local build

This project uses:

- **Make** for building, with all rules defined in Makefile
- **Python** for helper_scripts and ZHA quirks
- **Javascript** for Z2M converters
- **YAML** for the device database

**Linux is recommended.**  
We currently have bash scripts for Debian/Ubuntu to install dependencies with apt and automate building for multiple boards.  
They can easily be adapted for other distributions. (Please share your scripts)

1. Fork the repository and clone it
2. Run `make_scripts/make_install.sh` (one-time)
3. Enter virtual env to access python packages `source .venv/bin/activate`
4. Create **code_branch** from main (eg. newFeature)
5. Make changes
6. Build with `make_scripts/make_all.sh` or `make_scripts/make_debug_single.sh`
7. Run unit tests with `make tests`: [tests.md](./tests.md)
8. Perform device update and test: [flashing/](../flashing/)
9. Update `changelog_fw.md` (manual)
10. Commit changes (without generated files) and push
11. Create a Pull Request (**code_branch** ➡ **romasku/main**)

### 🐳 Dev Container build

As an alternative, dev containers provide a quick local setup without manually installing dependencies on your machine. They work across all platforms that support Linux containers and provide a consistent, isolated development environment. The container automatically installs all required dependencies and tools, configures VS Code extensions (C/C++ IntelliSense, Hex Editor, YAML), and activates the python virtual environment.

**Prerequisites:**
- [Docker](https://www.docker.com/get-started) or [Podman](https://podman.io/)
- [Visual Studio Code](https://code.visualstudio.com/) with [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

**Setup:**
1. Fork the repository and clone it
2. Open the project folder in VS Code
3. Click **"Reopen in Container"** (or Command Palette: `Dev Containers: Reopen in Container`)
4. Wait for container to build and install dependencies (first time takes a few minutes)

After the container is ready, continue with the local build workflow from step 4 onwards: create **code_branch**, make changes, build with make commands, run tests, etc.

### Available commands

| Command                               | Description                                               |
| ------------------------------------- | --------------------------------------------------------- |
| `make help`                           | Show all available make commands                          |
| `make setup`                          | Install all tools and dependencies                        |
| `make tests`                          | Run automated tests (builds stub first)                   |
| `make stub/build`                     | Build simulation environment for testing                  |
| `make stub/run`                       | Run interactive device simulation                         |
| `make telink/tools/all`               | Install Telink development tools                          |
| `make telink/build`                   | Build firmware for Telink hardware                        |
| `make silabs/tools/all`               | Install Silicon Labs development tools                    |
| `make silabs/build`                   | Build firmware for Silicon Labs hardware                  |
| `make silabs/install`                 | Flash firmware to connected device                        |
| `make tools/update_converters`        | Generate Zigbee2MQTT converters                           |
| `make tools/update_zha_quirk`         | Generate ZHA quirks                                       |
| `make tools/update_supported_devices` | Update [supported_devices.md](/docs/supported_devices.md) |
| `make tools/freeze_ota_links`         | Replace branch refs with commit IDs in OTA indexes        |
| `make tools/clean_z2m_index`          | Clear Zigbee2MQTT OTA index files                         |

> [!TIP]  
> Run `make help` to see all available commands, or use `make <platform>/help` (e.g., `make silabs/help`) for platform-specific options.

## Further reading

- [porting.md](./porting.md)
- [project_structure.md](./project_structure.md)
- [device_db_explained.md](./device_db_explained.md)
- [debugging.md](./debugging.md)
- [tests.md](./tests.md)
