[![GitHub stars](https://img.shields.io/github/stars/romasku/tuya-zigbee-switch.svg)](https://github.com/romasku/tuya-zigbee-switch/stargazers)
[![GitHub issues](https://img.shields.io/github/issues/romasku/tuya-zigbee-switch.svg)](https://github.com/romasku/tuya-zigbee-switch/issues)
[![StandWithUkraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](https://github.com/vshymanskyy/StandWithUkraine/blob/main/docs/README.md)

# 🔓 Custom firmware for Telink Tuya switches

Feature-rich custom firmware for Telink-based Tuya switches and switch-modules.  
Code is based on pvvx's [ZigbeeTLc](https://github.com/pvvx/ZigbeeTLc) firmware, huge thanks!

## 🔌 Supported devices

There are already **25+** [**supported_devices.md**](./docs/supported_devices.md).  
  
> [!TIP]   
> If your device contains a **supported Tuya Zigbee module** (ZT3L, ZTU, ZT2S), porting is relatively simple.  
> It consists of tracing (or guessing) the **board pinout**, adding an entry in `device_db.yaml` and rebuilding. 
>  
> Further reading:  
> [porting_to_new_device.md](./docs/porting_to_new_device.md), [device_db_explained.md](./docs/device_db_explained.md),  
> [recommended_devices.md](./docs/recommended_devices.md), [not_recommended_devices.md](./docs/not_recommended_devices.md).

## 🤔 Why?

The main driver for this project was a **frustrating bug in the factory firmware**:  
> When you pressed one button, the device **ignored input from the others** for ~0.5 seconds. As a result, pressing two buttons simultaneously toggled a single relay.

Users also consider this *the missing piece of a reliable smart home* because it allows **using a light switch as a Zigbee remote**.  
> Most of the cheap switches on the market do not allow **binding to other devices out-of-the-box.** 

## ✨ Features

- **Super fast reaction time** (compared to stock firmware)
- **Outgoing binds** (use switch to remotely control Zigbee lightbulbs - state & brightness)
- Configurable **Long press** for push-switches (custom action & duration)
- Custom **switch action modes**, allowing to synchonize switch position or binded devices with relay state
- Both **Router** & **EndDevice** modes for no-Neutral devices
- **Detached mode** (generate Zigbee events without triggering relays)
- **Power-on behavior** (on, off, previous, toggle)
- **Wireless flashing and updating** (OTA from original fw to custom fw, further OTA updates)
- Multiple **reset options** (5x switch press, on-board button)

## 📲 Flashing

The firmware can be installed:
- wirelessly on Z2M: [ota_flash.md](./docs/ota_flash.md) (recommended)
- wirelessly on old Z2M: [ota_flash_z2m_v1.md](./docs/ota_flash_z2m_v1.md)
- wirelessly on ZHA: [zha_ota.md](./docs/zha_ota.md)
- by wire: [flashing_via_wire.md](./docs/flashing_via_wire.md)

> [!TIP]  
> To switch between End Device and Router, follow [change_device_type.md](./docs/change_device_type.md)

## ❔ Frequently Asked Questions (FAQ)

Read here: [faq.md](./docs/faq.md) and feel free to ask more questions or suggest useful information.

## 📝 Changelog

Read the firmware release notes here: [changelog_fw.md](./docs/changelog_fw.md).  

> [!CAUTION]  
> **Always check the changelog before updating!**  
> Further versions could include breaking changes or require resetting the device!

## 🚨 ️Known issues

Read the known issues here: [known_issues.md](./docs/known_issues.md).

> [!WARNING]  
> **Stay up to date with the known issues to prevent bricking your device!**

## 🛠️ Contributing

**Welcome to the team!** Please read:  
- [building.md](./docs/building.md)
- [project_structure.md](./docs/project_structure.md)
- [device_db_explained.md](./docs/device_db_explained.md)
- [tests.md](./docs/tests.md)

## 🙏 Acknowledgements

- https://github.com/pvvx/ZigbeeTLc  
(fw for Telink-based temperature & humidity sensors) - the base of this project
- https://github.com/doctor64/tuyaZigbee  
(fw for some other Tuya Zigbee devices) - helpful examples
- https://medium.com/@omaslyuchenko  
for the **Hello Zigbee World** series - very useful references on how to program a Zigbee device

## ⭐ Star History

<a href="https://www.star-history.com/#romasku/tuya-zigbee-switch&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date" />
 </picture>
</a>
