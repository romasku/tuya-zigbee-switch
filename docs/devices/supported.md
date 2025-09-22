# Supported devices

**Z2M device name** usually represents a collection of devices.  
Even if they work the same, rebranded versions have different internals and different pinouts (and therefore require custom builds).  

**Zigbee Manufacturer** is the most reliable unique device identifier. If the TZ3000 id of your device is not on this list, it requires porting. 

For devices that contain a **supported Tuya Zigbee module** (ZTU, ZT2S, ZT3L), porting is relatively simple.  
It consists of tracing (or guessing) the **board pinout**, adding an entry in the `device_db.yaml` file and running the build action. 

Also read:  
- [contribute/porting_to_new_device.md](/docs/contribute/porting_to_new_device.md)
- [recommended.md](./recommended.md)
- [not_recommended.md](./not_recommended.md)

| Z2M device name | Vendor name | Zigbee Manufacturer | Type | Status | Issue |
| --- | --- | --- | --- | --- | --- |
| [TS0012_switch_module](https://www.zigbee2mqtt.io/devices/TS0012_switch_module.html) | GIRIER TS0012, OXT  | _TZ3000_jl7qyupf | router / end_device | Supported |    -  | 
| [WHD02](https://www.zigbee2mqtt.io/devices/WHD02.html) | No-name 1 gang switch  | _TZ3000_skueekg3 | router | Supported |    -  | 
| [TS0002_basic](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | OXT TS0001, probably other rebrands  | _TZ3000_01gpyda5 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/6)  | 
| [TS0002_basic](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | OXT 2-gang switch  | _TZ3000_bvrlqyj7 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/49)  | 
| [TS0011_switch_module](https://www.zigbee2mqtt.io/devices/TS0011_switch_module.html) | GIRIER TS0011, OXT TS0011  | _TZ3000_ji4araar | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/4)  | 
| [TS0001_switch_module](https://www.zigbee2mqtt.io/devices/TS0001_switch_module.html) | OXT TS0001, probably other rebrands  | _TZ3000_tqlv4ug4 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/6)  | 
| [TS0002_basic](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | GIRIER 2 Gang  | _TZ3000_zmy4lslw | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/29)  | 
| [ZWSM16-1-Zigbee](https://www.zigbee2mqtt.io/devices/ZWSM16-1-Zigbee.html) | AVATTO ZWSM16-1 (blue socket)  | _TZ3000_4rbqgcuv | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [ZWSM16-2-Zigbee](https://www.zigbee2mqtt.io/devices/ZWSM16-2-Zigbee.html) | AVATTO ZWSM16-2 (blue socket)  | _TZ3000_mtnpt6ws | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [ZWSM16-3-Zigbee](https://www.zigbee2mqtt.io/devices/ZWSM16-3-Zigbee.html) | AVATTO ZWSM16-3 (blue socket)  | _TZ3000_hbic3ka3 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/56)  | 
| [ZWSM16-4-Zigbee](https://www.zigbee2mqtt.io/devices/ZWSM16-4-Zigbee.html) | AVATTO ZWSM16-4 (blue socket)  | _TZ3000_5ajpkyq6 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [ZS-EUB_2gang](https://www.zigbee2mqtt.io/devices/ZS-EUB_2gang.html) | Moes TS0012 (2 gang switch)  | _TZ3000_18ejxno0 | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/14)  | 
| [TS0012](https://www.zigbee2mqtt.io/devices/TS0012.html) | Bseed TS0012 (2 gang switch)  | _TZ3000_f2slq5pj | router / end_device | In progress |   [link](https://github.com/romasku/tuya-zigbee-switch/pull/23)  | 
| [TS0012](https://www.zigbee2mqtt.io/devices/TS0012.html) | Bseed TS0012 (2 gang switch)  | _TZ3000_xk5udnd6 | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/51)  | 
| [LZWSM16-2](https://www.zigbee2mqtt.io/devices/LZWSM16-2.html) | Avatto LZWSM16-2 (blue socket)  | _TZ3000_ljhbw1c9 | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/16)  | 
| [LZWSM16-3](https://www.zigbee2mqtt.io/devices/LZWSM16-3.html) | AVATTO LZWSM16-3 (blue socket)  | _TZ3000_avotanj3 | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/135)  | 
| [WHD02](https://www.zigbee2mqtt.io/devices/WHD02.html) | Aubess WHD02  | _TZ3000_46t1rvdu | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/18)  | 
| [ZS-EUB_1gang](https://www.zigbee2mqtt.io/devices/ZS-EUB_1gang.html) | Moes TS0011 (1 gang switch)  | _TZ3000_hhiodade | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/14)  | 
| [TS0013](https://www.zigbee2mqtt.io/devices/TS0013.html) | Moes TS0013 (3 gang switch)  | _TZ3000_qewo8dlz | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/14)  | 
| [TS0012](https://www.zigbee2mqtt.io/devices/TS0012.html) | Zemismart 2 gang switch (needs confirmation)  | _TZ3000_zmlunnhy | router / end_device | In progress |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/19)  | 
| [LZWSM16-1](https://www.zigbee2mqtt.io/devices/LZWSM16-1.html) | Avatto LZWSM16-1 (blue socket)  | _TZ3000_hbxsdd6k | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/9)  | 
| [TS0003](https://www.zigbee2mqtt.io/devices/TS0003.html) | Moes MS-104CZ  | _TZ3000_pfc7i3kt | router | Supported |    -  | 
| [TS0003](https://www.zigbee2mqtt.io/devices/TS0003.html) | Bseed 3-gang touch switch with Neutral  | _TZ3000_7aqaupa9 | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/125)  | 
| [TS0003_switch_module_2](https://www.zigbee2mqtt.io/devices/TS0003_switch_module_2.html) | IHSENO 3-gang Neutral  | _TZ3000_mhhxxjrs | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/85)  | 
| [TS0004_switch_module_2](https://www.zigbee2mqtt.io/devices/TS0004_switch_module_2.html) | IHSENO 4-gang Neutral  | _TZ3000_knoj8lpk | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/105)  | 
| [ZB08](https://www.zigbee2mqtt.io/devices/ZB08.html) | Girier-ZB08  | _TZ3000_ypgri8yz | router / end_device | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/37)  | 
| [TS0004_switch_module](https://www.zigbee2mqtt.io/devices/TS0004_switch_module.html) | Avatto 4 gang switch module with N  | _TZ3000_ltt60asa | router | In progress |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/42)  | 
| [TS0004_switch_module](https://www.zigbee2mqtt.io/devices/TS0004_switch_module.html) | No-name 4 gang switch module with N  | _TZ3000_mmkbptmx | router | Supported |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/66)  | 
| [TS0003_switch_3_gang](https://www.zigbee2mqtt.io/devices/TS0003_switch_3_gang.html) | Avatto Zigbee 3 Gang (ZTS02RD-US-W3)  | _TZ3000_avky2mvc | router | In progress |   [link](https://github.com/romasku/tuya-zigbee-switch/issues/41)  | 

