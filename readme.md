[![GitHub stars](https://img.shields.io/github/stars/romasku/tuya-zigbee-switch.svg?style=flat&label=Stars&color=yellow)](https://github.com/romasku/tuya-zigbee-switch/stargazers)
[![GitHub issues](https://img.shields.io/github/issues/romasku/tuya-zigbee-switch.svg?label=Issues)](https://github.com/romasku/tuya-zigbee-switch/issues)
[![StandWithUkraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](https://github.com/vshymanskyy/StandWithUkraine/blob/main/docs/README.md)
[![Discord](https://img.shields.io/discord/1405486711412359278.svg?logo=discord&logoColor=white&label=Chat&color=blue)](https://discord.gg/4HAg2Fr565)

# 🔓 Custom firmware for Tuya Zigbee switches

Feature-rich custom firmware for Telink/Silabs Zigbee switches, modules, sockets

- Already **60+** [devices/supported.md](/docs/devices/supported.md)
- Port new devices: [contribute/porting.md](/docs/contribute/porting.md)

## 🤔 Why?

The main driver for this project was a **frustrating bug in the factory firmware:**

> When you pressed one button, the device shortly ignored input from the others.  
> As a result, simultaneously **pressing two buttons toggled a single relay.**

Users also consider this _the missing piece of a reliable smart home,_ because it allows **using a light switch as a Zigbee remote**.

> Most of the cheap switches on the market do not allow **binding to other devices** out-of-the-box.

## ✨ Features

- **Super fast reaction time** (compared to stock firmware)
- **Outgoing binds** (use switch to remotely control Zigbee lightbulbs - state & brightness)
- Supports **all button types**: toggle, momentary NO, momentary NC
- Configurable **Long press** for push-switches (custom action & duration)
- Custom **switch action modes**, allowing to synchronize switch position or binded devices with relay state
- Both **Router** & **EndDevice** modes for no-Neutral devices
- **Detached mode** (generate Zigbee events without triggering relays)
- **Power-on behavior** (on, off, previous, toggle)
- **Wireless flashing and updating** (OTA from original fw to custom fw, further OTA updates)
- Multiple **reset options** (10x switch press, on-board button)

## 📲 Flashing

If your device is already on [devices/supported.md](/docs/devices/supported.md), the firmware can be **installed and updated**:

- wirelessly on Z2M / ZHA: [updating.md](/docs/updating.md)
- by wire: [contribute/flashing_via_wire.md](/docs/contribute/flashing_via_wire.md)
- by wire for silicon labs MCU: [contribute/flashing_via_wire_silabs.md](/docs/contribute/flashing_via_wire_silabs.md)

Otherwise, check [contribute/porting.md](/docs/contribute/porting.md).

## 📝 Changelog

Read the firmware release notes here: [changelog_fw.md](/docs/changelog_fw.md).

## 🚨 ️Known issues

Stay up to date with the [known_issues.md](/docs/known_issues.md) to prevent bricking your device!

## ❓ Frequently Asked Questions (FAQ)

Read the [faq.md](/docs/faq.md) and feel free to ask more questions or suggest useful information.  
Also read [endpoints.md](/docs/usage/endpoints.md) for information about groups and binding.  
To switch between EndDevice and Router, follow [change_device_type.md](/docs/usage/change_device_type.md).

## Discord

Join the discussion, ask for help or just follow the news on:

[![Discord](https://discord.com/api/guilds/1405486711412359278/widget.png?style=banner3)](https://discord.gg/4HAg2Fr565)

Keep important topics on GitHub!

## 🛠️ Building and Contributing

**Welcome to the team!** Please read:

- [porting.md](/docs/contribute/porting.md)
- [building.md](/docs/contribute/building.md)
- [project_structure.md](/docs/contribute/project_structure.md)
- [device_db_explained.md](docs/contribute/device_db_explained.md)
- [debugging.md](/docs/contribute/debugging.md)
- [tests.md](/docs/contribute/tests.md)

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
