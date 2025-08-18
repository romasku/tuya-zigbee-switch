[![GitHub stars](https://img.shields.io/github/stars/romasku/tuya-zigbee-switch.svg)](https://github.com/romasku/tuya-zigbee-switch/stargazers)
[![GitHub issues](https://img.shields.io/github/issues/romasku/tuya-zigbee-switch.svg)](https://github.com/romasku/tuya-zigbee-switch/issues)
[![StandWithUkraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](https://github.com/vshymanskyy/StandWithUkraine/blob/main/docs/README.md)

# 🔓 Custom firmware for Telink Tuya switches

Feature-rich custom firmware for Telink-based Tuya switches / switch-modules.  
Code is based on pvvx's [ZigbeeTLc](https://github.com/pvvx/ZigbeeTLc) firmware, huge thanks!

## 🔌 Supported devices

Note that rebranded versions may have different internals, requiring different pinouts (and therefore custom builds).  
"Zigbee Manufacturer" is the most reliable device identifier.

| Z2M device name | Vendor name | Zigbee Manufacturer | Type | Status | Issue |
| --- | --- | --- | --- | --- | --- |
| [TS0012_switch_module](https://www.zigbee2mqtt.io/devices/TS0012_switch_module.html) | GIRIER TS0012, OXT  | _TZ3000_jl7qyupf | router / end_device | Supported |    -  | 
| [WHD02](https://www.zigbee2mqtt.io/devices/WHD02.html) | No-name 1 gang switch  | _TZ3000_skueekg3 | router | Supported |    -  | 
| [TS0002_basic](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | OXT TS0001, probably other rebrands  | _TZ3000_01gpyda5 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/6)  | 
| [TS0002_basic](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | OXT 2-gang switch  | _TZ3000_bvrlqyj7 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/49)  | 
| [TS0011_switch_module](https://www.zigbee2mqtt.io/devices/TS0011_switch_module.html) | GIRIER TS0011, OXT TS0011  | _TZ3000_ji4araar | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/4)  | 
| [TS0001_switch_module](https://www.zigbee2mqtt.io/devices/TS0001_switch_module.html) | OXT TS0001, probably other rebrands  | _TZ3000_tqlv4ug4 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/6)  | 
| [TS0002_basic](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | GIRIER 2 Gang  | _TZ3000_zmy4lslw | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/29)  | 
| [ZWSM16-1-Zigbee](https://www.zigbee2mqtt.io/devices/ZWSM16-1-Zigbee.html) | AVATTO ZWSM16-1-Zigbee (blue socket)  | _TZ3000_4rbqgcuv | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [ZWSM16-2-Zigbee](https://www.zigbee2mqtt.io/devices/ZWSM16-2-Zigbee.html) | AVATTO ZWSM16-2-Zigbee (blue socket)  | _TZ3000_mtnpt6ws | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [ZWSM16-3-Zigbee](https://www.zigbee2mqtt.io/devices/ZWSM16-3-Zigbee.html) | AVATTO ZWSM16-3-Zigbee (blue socket)  | _TZ3000_hbic3ka3 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/56)  | 
| [ZWSM16-4-Zigbee](https://www.zigbee2mqtt.io/devices/ZWSM16-4-Zigbee.html) | AVATTO ZWSM16-4-Zigbee (blue socket)  | _TZ3000_5ajpkyq6 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [TS0012](https://www.zigbee2mqtt.io/devices/TS0012.html) | Moes TS0012 (2 gang switch)  | _TZ3000_18ejxno0 | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/14)  | 
| [TS0012](https://www.zigbee2mqtt.io/devices/TS0012.html) | Bseed TS0012 (2 gang switch)  | _TZ3000_f2slq5pj | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/pull/23)  | 
| [TS0012](https://www.zigbee2mqtt.io/devices/TS0012.html) | Bseed TS0012 (2 gang switch)  | _TZ3000_xk5udnd6 | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/51)  | 
| [TS0012_switch_module](https://www.zigbee2mqtt.io/devices/TS0012_switch_module.html) | Avatto TS0012  | _TZ3000_ljhbw1c9 | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/16)  | 
| [WHD02](https://www.zigbee2mqtt.io/devices/WHD02.html) | Aubess WHD02  | _TZ3000_46t1rvdu | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/18)  | 
| [ZS-EUB_1gang](https://www.zigbee2mqtt.io/devices/ZS-EUB_1gang.html) | Moes TS0011 (1 gang switch)  | _TZ3000_hhiodade | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/14)  | 
| [TS0013](https://www.zigbee2mqtt.io/devices/TS0013.html) | Moes TS0013 (3 gang switch)  | _TZ3000_qewo8dlz | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/14)  | 
| [TS0012](https://www.zigbee2mqtt.io/devices/TS0012.html) | Internet search says this is Zemismart 2 gang switch, needs confirmation  | _TZ3000_zmlunnhy | router / end_device | In progress |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/19)  | 
| [LZWSM16-1](https://www.zigbee2mqtt.io/devices/LZWSM16-1.html) | Avatto TS0011  | _TZ3000_hbxsdd6k | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/16)  | 
| [TS0003](https://www.zigbee2mqtt.io/devices/TS0003.html) | Moes MS-104CZ  | _TZ3000_pfc7i3kt | router | Supported |    -  | 
| [TS0003_switch_module_2](https://www.zigbee2mqtt.io/devices/TS0003_switch_module_2.html) | IHSENO 3-gang Neutral  | _TZ3000_mhhxxjrs | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/85)  | 
| [TS0004_switch_module_2](https://www.zigbee2mqtt.io/devices/TS0004_switch_module_2.html) | IHSENO 4-gang Neutral  | _TZ3000_knoj8lpk | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/105)  | 
| [ZB08](https://www.zigbee2mqtt.io/devices/ZB08.html) | Girier-ZB08  | _TZ3000_ypgri8yz | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/37)  | 
| [TS0004_switch_module](https://www.zigbee2mqtt.io/devices/TS0004_switch_module.html) | Avatto 4 gang switch module with N  | _TZ3000_ltt60asa | router | WIP, check issue for current status. |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/42)  | 
| [TS0004_switch_module](https://www.zigbee2mqtt.io/devices/TS0004_switch_module.html) | No-name 4 gang switch module with N  | _TZ3000_mmkbptmx | router | Untested |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/66)  | 
| [TS0003_switch_3_gang](https://www.zigbee2mqtt.io/devices/TS0003_switch_3_gang.html) | Avatto Zigbee 3 Gang (ZTS02RD-US-W3)  | _TZ3000_avky2mvc | router | WIP, expiremntal, see issue. |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/41)  | 

If you device is not supported, but it is some Tuya switch module, please check: [porting_to_new_device.md](./docs/porting_to_new_device.md)

## 🤔 Why?

The main driver for this project was a **frustrating bug in the factory firmware**:  
> When you pressed one button, the device ignored input from the others for ~0.5 seconds. As a result, pressing two buttons simultaneously toggled a single relay.

Users also consider this *the missing piece of a reliable smart home* because it allows **using a light switch as a Zigbee remote**.  
(There are no switches on the market that allow binding **to** other devices out-of-the-box.) 

## ✨ Features

- **Super fast reaction time** (compared to stock firmware)
- **Outgoing binds** (use switch to remotely control Zigbee lightbulbs - state & brightness)
- Configurable **Long press** for push-switches (custom action & duration)
- Both **Router** & **EndDevice** modes for no-Neutral devices
- **Detached mode** (generate Zigbee events without triggering relays)
- **Power-on behavior** (on, off, previous, toggle)
- Multiple **reset options** (5x switch press, on-board button)
- Custom **switch action modes**, allowing to synchonize switch position or binded devices with relay state

## 📲 Flashing

The firmware can be installed:
- wirelessly on Z2M: [ota_flash.md](./docs/ota_flash.md) (recommended)
- wirelessly on old Z2M: [ota_flash_z2m_v1.md](./docs/ota_flash_z2m_v1.md)
- wirelessly on ZHA: [zha_ota.md](./docs/zha_ota.md)
- by wire: [flashing_via_wire.md](./docs/flashing_via_wire.md)

To switch between End Device and Router, follow [change_device_type.md](./docs/change_device_type.md)

## ❔ Frequently Asked Questions (FAQ)

Read here: [faq.md](./docs/faq.md)

## 📝 Changelog

Latest version: **v1.0.18**  
Read the firmware release notes here: [changelog_fw.md](./docs/changelog_fw.md)

## 🛠️ Building

Read [building.md](./docs/building.md)

## 🙏 Acknowledgements

- https://github.com/pvvx/ZigbeeTLc (firmware for telink based ATC) as this was base for this project
- https://github.com/doctor64/tuyaZigbee (firmware for some other Tuya Zigbee devices) for some helpful examples
- https://medium.com/@omaslyuchenko for **Hello Zigbee World** series, that contain very usefull reference on how to implement a Zigbee device 

## ⭐ Star History

<a href="https://www.star-history.com/#romasku/tuya-zigbee-switch&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=romasku/tuya-zigbee-switch&type=Date" />
 </picture>
</a>
