[![GitHub stars](https://img.shields.io/github/stars/romasku/tuya-zigbee-switch.svg)](https://github.com/romasku/tuya-zigbee-switch/stargazers)
[![GitHub issues](https://img.shields.io/github/issues/romasku/tuya-zigbee-switch.svg)](https://github.com/romasku/tuya-zigbee-switch/issues)
[![StandWithUkraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](https://github.com/vshymanskyy/StandWithUkraine/blob/main/docs/README.md)

# Custom firmware for Tuya switch

A custom firmware for Tuya telink based switch module. Code is based on pvvx's [ZigbeeTLc](https://github.com/pvvx/ZigbeeTLc) firmware, huge thanks!

## Supported devices

Note that rebranded versions may have different internals and may not work. "Zigbee Manufacturer" is the most reliable identifier of the device.

| Z2M device name | Vendor name | Zigbee Manufacturer | Type | Status | Issue |
| --- | --- | --- | --- | --- | --- |
| [TS0012_switch_module](https://www.zigbee2mqtt.io/devices/TS0012_switch_module.html) | GIRIER TS0012, OXT  | _TZ3000_jl7qyupf | router / end_device | Supported |    -  | 
| [WHD02](https://www.zigbee2mqtt.io/devices/WHD02.html) | No-name 1 gang switch  | _TZ3000_skueekg3 | router | Supported |    -  | 
| [TS0002_basic](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | OXT TS0001, probably other rebrands  | _TZ3000_01gpyda5 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/6)  | 
| [TS0011_switch_module](https://www.zigbee2mqtt.io/devices/TS0011_switch_module.html) | GIRIER TS0011, OXT TS0011  | _TZ3000_ji4araar | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/4)  | 
| [TS0001_switch_module](https://www.zigbee2mqtt.io/devices/TS0001_switch_module.html) | OXT TS0001, probably other rebrands  | _TZ3000_tqlv4ug4 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/6)  | 
| [TS0002_basic](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | GIRIER 2 Gang  | _TZ3000_zmy4lslw | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/29)  | 
| [TS0001_switch_module](https://www.zigbee2mqtt.io/devices/TS0001_switch_module.html) | Avatto TS0001  | _TZ3000_4rbqgcuv | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [TS0002_limited](https://www.zigbee2mqtt.io/devices/TS0002_limited.html) | Avatto TS0002  | _TZ3000_mtnpt6ws | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [TS0003_switch_module_2](https://www.zigbee2mqtt.io/devices/TS0003_switch_module_2.html) | Avatto TS0003  | _TZ3000_hbic3ka3 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/56)  | 
| [TS0004_switch_module_2](https://www.zigbee2mqtt.io/devices/TS0004_switch_module_2.html) | Avatto TS0004  | _TZ3000_5ajpkyq6 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
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
| [ZB08](https://www.zigbee2mqtt.io/devices/ZB08.html) | Girier-ZB08  | _TZ3000_ypgri8yz | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/37)  | 
| [TS0004_switch_module](https://www.zigbee2mqtt.io/devices/TS0004_switch_module.html) | Avatto 4 gang switch module with N  | _TZ3000_ltt60asa | router | WIP, check issue for current status. |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/42)  | 
| [TS0004_switch_module](https://www.zigbee2mqtt.io/devices/TS0004_switch_module.html) | No-name 4 gang switch module with N  | _TZ3000_mmkbptmx | router | Untested |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/66)  | 
| [TS0003_switch_3_gang](https://www.zigbee2mqtt.io/devices/TS0003_switch_3_gang.html) | Avatto Zigbee 3 Gang (ZTS02RD-US-W3)  | _TZ3000_avky2mvc | router | WIP, expiremntal, see issue. |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/41)  | 

If you device is not supported, but it is some Tuya switch module, please check [the porting guide](./docs/porting_to_new_device.md).

## Why?

The main driver for this project was the following factory firmware bug: if one button is pressed, the device ignores clicks to other buttons for ~0.5 seconds. The most frustrating consequence is that pressing both buttons at the same time turns only one relay on.

## Features

- Detached mode, e.g. switch doesn't trigger relay but only generates events via Zigbee
- Bind switch to light bulb
- Long press for momentary switches with configurable duration
- Router/EndDevice modes for no-neutral devices
- Super fast reaction time (compared to the factory firmware)
- 5 quick presses to reset the device
- Power-on behavior 
- Switch modes:  
ON_OFF, OFF_ON, TOGGLE_SIMPLE, TOGGLE_SMART_SYNC, TOGGLE_SMART_OPPOSITE  
allowing to synchonize switch position with relay state

## Building

Only on linux:

```
make install
make
```

## Flashing

Firmware can be [flashed via OTA](./docs/ota_flash.md). If you still use zigbee2mqtt 1.x, use [this guide](./docs/ota_flash_z2m_v1.md)

To switch between End Device and Router follow [this guide](./docs/change_device_type.md)

To flash via wire, follow [this guide](./docs/flashing_via_wire.md)

## Changelog

### v1.0.17

- Fix once again power on behavior = OFF not working if toggle in pressed state during boot

### v1.0.16

- Add new toggle modes: TOGGLE_SMART_SYNC/TOGGLE_SMART_OPPOSITE (requires re-download of `switch_custom.js`)

### v1.0.15

- Add support for Zigbee groups. Read [doc](./docs/endpoints.md) for details about endpoints.

### v1.0.14

- Improve code logic for Indicator LED on for switches

### v1.0.13

- Fix power on behavior = OFF not working if toggle in pressed state during boot
- Add way to control network state led state (requires re-download of `switch_custom.js`)

### v1.0.12

- Fix led indicator state in manual mode not preserved after reboot
- Add forced device announcement after boot to make sure device is seen as "available" as soon as it boots
- Restored device pictures in z2m (requires re-download of `switch_custom.js`)
- Cleaned-up z2m converter (fix typos, inconsistent names, etc). **Warning!** This may break your automations as it changes 
  property names (requires re-download of `switch_custom.js`)

### v1.0.11

- Improve join behaviour by decreasing timeout between tries to join
- Fix leave network: now device will send LeaveNetwork command properly
- Display firmware version in a human-readable form

### v1.0.10

- Add support for bi-stable relays contolled by 2 pins
- Fix Led indicator mode not preserved after reboot

### v1.0.9

- Fix reporting of indicator led status

### v1.0.8

- Add support for indicator leds
- Add way to force momentary mode as default via config

### v1.0.7

- Add SUSPEND-based sleep to EndDevice firmware to decrease power usage ~10x

### v1.0.6

- Add way to change device pinout on the fly, to allow easier porting of firmware 

### v1.0.5

- Keep status LED on when device is connected
- Add separate firmwares for End Device/Router
- Improve device boot time significantly by removing unnecessary logs 

### v1.0.4

- Fix bug that caused report to be sent every second

### v1.0.3

- Add support of statup behaviour: ON, OFF, TOGGLE, PREVIOUS
- Add support of button actions: 'released', 'press', 'long_press'. This is only useful for momentary (doorbell-like) switches.

### v1.0.2

- Add way to reset the device by pressing any switch button 5 times in a row 
- Fix support for ON_OFF, OFF_ON actions


## Acknowledgements

- https://github.com/pvvx/ZigbeeTLc (firmware for telink based ATC) as this was base for this project
- https://github.com/doctor64/tuyaZigbee (firmware for some other Tuya Zigbee devices) for some helpful examples
- https://medium.com/@omaslyuchenko for **Hello Zigbee World** series, that contain very usefull reference on how to implement a Zigbee device 
