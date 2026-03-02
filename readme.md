[![GitHub stars](https://img.shields.io/github/stars/romasku/tuya-zigbee-switch.svg?style=flat&label=Stars&color=yellow)](https://github.com/romasku/tuya-zigbee-switch/stargazers)
[![GitHub issues](https://img.shields.io/github/issues/romasku/tuya-zigbee-switch.svg?label=Issues)](https://github.com/romasku/tuya-zigbee-switch/issues)
[![StandWithUkraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](https://github.com/vshymanskyy/StandWithUkraine/blob/main/docs/README.md)
[![Discord](https://img.shields.io/discord/1405486711412359278.svg?logo=discord&logoColor=white&label=Chat&color=blue)](https://discord.gg/4HAg2Fr565)

# 🔓 Custom firmware for Tuya Zigbee switches

Feature-rich custom firmware for Telink/Silabs Zigbee switches, modules, sockets

- Already **130+** [supported_devices.md](./docs/supported_devices.md)
- Port new devices: [contribute/porting.md](./docs/contribute/porting.md)

## 🤔 Why?

The main driver for this project was a **frustrating bug in the factory firmware:**

> When you pressed one button, the device shortly ignored input from the others.  
> As a result, simultaneously **pressing two buttons toggled a single relay.**

Users also consider this _the missing piece of a reliable smart home,_ because it allows **using a light switch as a Zigbee remote**.

> Most cheap switches on the market don't allow **binding to other devices** out-of-the-box.

## ✨ Features

### Already implemented
- **Super fast reaction time** (choose action moment: press / release)
- **Detached mode** (unlink switch and relay)
- **Outgoing binds**  (remotely control Zigbee lights - state & brightness)
- Supports **all button types**: toggle, momentary NO, momentary NC
- Configurable **Long press** for push-switches (action & duration)
- Custom **switch action modes** (sync: switch position - relay state - bound devices)
- Both **Router** & **EndDevice** modes for no-Neutral devices
- **Power-on behavior** (on, off, previous, toggle)
- **Wireless flashing and updating** (OTA from original fw, further OTA updates)
- Multiple **reset options** (10x switch press, on-board button)

### Work in progress
- Wireless switches (battery-powered remotes)
- FW-level multi-press (double or triple click)
- Countdown timers (on_with_timed_off)
- Inching (pulse relay output)
- Scenes (send and receive)
- Power monitoring
- Curatain modules
- Touchlink
- Integrate converters with Z2M

## 📲 Installation

If your device is already on [**supported_devices.md**](./docs/supported_devices.md), the firmware can be **installed**:

- wirelessly on Z2M / ZHA (only Telink devices): [updating.md](./docs/updating.md)
- by wire: [flashing/](./docs/flashing/)

Otherwise, check [contribute/porting.md](./docs/contribute/porting.md).

## 📑 Documentation

**Information and diagrams are available in [docs/](./docs/)**

Some quick links:
- ❓ [**faq.md**](./docs/faq.md) ⬅ *Troubleshoot*
- 🚨 ️[known_issues.md](./docs/known_issues.md)
- 📝 [changelog_fw.md](./docs/changelog_fw.md)
- 🛠️ [contribute/](./docs/contribute/)
- ⚙️ [usage/](./docs/usage/)
  - [endpoints.md](./docs/usage/endpoints.md) 
  - [change_device_type.md](./docs/usage/change_device_type.md)

## 💬 Chat

Discuss, troubleshoot and follow the updates on Discord 🙂

[![Discord](https://discord.com/api/guilds/1405486711412359278/widget.png?style=banner3)](https://discord.gg/4HAg2Fr565)

## 🙏 Acknowledgements

- [doctor64/tuyaZigbee](https://github.com/doctor64/tuyaZigbee)  
  ⤷ fw for other Tuya Zigbee devices (helpful examples)
- [medium.com/@omaslyuchenko](https://medium.com/@omaslyuchenko)  
  ⤷ **Hello Zigbee World** series (very useful Zigbee programming guides)

## ⭐ Star History

<a href="https://www.star-history.com/#romasku/tuya-zigbee-switch&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date" />
 </picture>
</a>
