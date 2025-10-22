# Supported devices

> [!NOTE]  
> **Z2M page** usually represents a collection of devices.  
> Even if they work the same, rebranded versions have different internals and pinouts  
> (therefore requiring custom builds).  

> [!IMPORTANT]  
> **Zigbee Manufacturer** is the most reliable unique Tuya device identifier.  
> **If the TZ3000 id of your device is not on this list, it requires [contribute/porting.md](/docs/contribute/porting.md)!**  
>
> For devices that contain a **supported Tuya Zigbee module** (ZTU, ZT2S, ZT3L), porting is relatively simple.  
> It consists of tracing (or guessing) the **board pinout**, adding an entry in the [`device_db.yaml`](/device_db.yaml) file and running the build action.  

**Also read:** [recommended.md](./recommended.md) & [not_recommended.md](./not_recommended.md)  

### Legend

| Symbol | Meaning            |                    |                      |                    |                |
| :----: | ------------------ | ------------------ | -------------------- | ------------------ | -------------- |
|   🚧   | Status             | 🟩 Fully supported | 🟨 Mostly supported  | 🟧 In progress     | 🟥 Unsupported |
|   📦   | Build              | ✔️ Available       | ❌ Unavailable       |                    |                |
|   ⚡   | Power              | 🔌 Mains           | 🔋 Battery           | 🔱 USB             |  ❓ Unknown    |

<!-------------------------------------------------------------------
  This page (`supported.md`) is generated. 
  
  Do not edit it directly! Instead, edit:
  - `device_db.yaml`                - add or edit devices
  - `supported.md.jinja`            - update the template
  - `make_supported.py`             - update generation script

  Generate with: `make supported`
-------------------------------------------------------------------->

### Device list

| 🚧 | 📦 | ⚡ | Zigbee Manufacturer | Name 🔗 Z2M page | Store | Threads | Status |
| -- | -- | -- | :-----------------: | ---------------- | ----: | ------: | ------ |
| 🟩 | ✔️ | 🔌 | `_TZ3000_46t1rvdu`  | [Aubess WHD02](https://www.zigbee2mqtt.io/devices/WHD02.html) |   | [`#018`](https://github.com/romasku/tuya-zigbee-switch/issues/18) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_lmlsduws`  | [Aubess TMZ02](https://www.zigbee2mqtt.io/devices/TMZ02.html) | [`AlEx`](https://www.aliexpress.com/item/1005005748264739.html) | [`#153`](https://github.com/romasku/tuya-zigbee-switch/pull/153) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_lvhy15ix`  | [Aubess 3-gang](https://www.zigbee2mqtt.io/devices/TS0003_switch_module_2.html) | [`Amzn`](https://www.amazon.co.uk/dp/B0DKC2CRFJ) [`AlEx`](https://www.aliexpress.com/item/1005005748264739.html) | [`#151`](https://github.com/romasku/tuya-zigbee-switch/pull/151) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_mmkbptmx`  | [Aubess 4-gang](https://www.zigbee2mqtt.io/devices/TS0004_switch_module.html) | [`AlEx`](https://www.aliexpress.com/item/1005005748264739.html) | [`#066`](https://github.com/romasku/tuya-zigbee-switch/issues/66) | Supported | 
| 🟨 | ✔️ | 🔌 | `_TZ3000_avky2mvc`  | [AVATTO 3-gang touch switch](https://www.zigbee2mqtt.io/devices/TS0003_switch_3_gang.html) | [`AlEx`](https://www.aliexpress.com/item/1005007097427150.html) | [`#041`](https://github.com/romasku/tuya-zigbee-switch/issues/41) | Indicator LEDs can be controlled with C4? | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_4rbqgcuv`  | [AVATTO ZWSM16-1](https://www.zigbee2mqtt.io/devices/ZWSM16-1-Zigbee.html) | [`AlEx`](https://www.aliexpress.com/item/1005007247647375.html) | [`#009`](https://github.com/romasku/tuya-zigbee-switch/issues/9) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_mtnpt6ws`  | [AVATTO ZWSM16-2](https://www.zigbee2mqtt.io/devices/ZWSM16-2-Zigbee.html) | [`AlEx`](https://www.aliexpress.com/item/1005007247647375.html) | [`#009`](https://github.com/romasku/tuya-zigbee-switch/issues/9) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_hbic3ka3`  | [AVATTO ZWSM16-3](https://www.zigbee2mqtt.io/devices/ZWSM16-3-Zigbee.html) | [`AlEx`](https://www.aliexpress.com/item/1005007247647375.html) | [`#056`](https://github.com/romasku/tuya-zigbee-switch/issues/56) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_5ajpkyq6`  | [AVATTO ZWSM16-4](https://www.zigbee2mqtt.io/devices/ZWSM16-4-Zigbee.html) | [`AlEx`](https://www.aliexpress.com/item/1005007247647375.html) | [`#009`](https://github.com/romasku/tuya-zigbee-switch/issues/9) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_hbxsdd6k`  | [AVATTO LZWSM16-1](https://www.zigbee2mqtt.io/devices/LZWSM16-1.html) | [`AlEx`](https://www.aliexpress.com/item/1005007247647375.html) | [`#009`](https://github.com/romasku/tuya-zigbee-switch/issues/9) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_ljhbw1c9`  | [AVATTO LZWSM16-2](https://www.zigbee2mqtt.io/devices/LZWSM16-2.html) | [`AlEx`](https://www.aliexpress.com/item/1005007247647375.html) | [`#016`](https://github.com/romasku/tuya-zigbee-switch/issues/16) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_avotanj3`  | [AVATTO LZWSM16-3](https://www.zigbee2mqtt.io/devices/LZWSM16-3.html) | [`AlEx`](https://www.aliexpress.com/item/1005007247647375.html) | [`#135`](https://github.com/romasku/tuya-zigbee-switch/issues/135) | Supported | 
| 🟧 | ✔️ | 🔌 | `_TZ3000_b28wrpvx`  | [BSEED PM socket](https://www.zigbee2mqtt.io/devices/TS011F_plug_1_2.html) | [`AlEx`](https://www.aliexpress.com/item/1005004402438527.html) [`AlEx`](https://www.aliexpress.com/i/1005005835434423.html) [`AlEx`](https://www.aliexpress.com/item/1005003350696939.html) | [`#145`](https://github.com/romasku/tuya-zigbee-switch/issues/145) | PM not implemented | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_o1jzcxou`  | [BSEED socket](https://www.zigbee2mqtt.io/devices/_TZ3000_o1jzcxou.html) | [`AlEx`](https://www.aliexpress.com/item/1005004402438527.html) | [`#145`](https://github.com/romasku/tuya-zigbee-switch/issues/145) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3002_zjuvw9zf`  | [BSEED 2-gang switch](https://www.zigbee2mqtt.io/devices/EC-GL86ZPCS21.html) | [`AlEx`](https://www.aliexpress.com/item/1005008749674564.html) [`BSEED`](https://www.bseed.com/products/bseed-smart-zigbee-light-switch-with-neutral-hub-required-switch-work-with-tuya-alexa?variant=46312889057435) | [`#157`](https://github.com/romasku/tuya-zigbee-switch/issues/157) | Supported | 
| 🟨 | ✔️ | 🔌 | `_TZ3000_f2slq5pj`  | [BSEED 2-gang touch switch 🅰](https://www.zigbee2mqtt.io/devices/TS0012.html) |   | [`#023`](https://github.com/romasku/tuya-zigbee-switch/pull/23) | Pin C3 disables both LEDs ? | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_xk5udnd6`  | [BSEED 2-gang touch switch 🅱](https://www.zigbee2mqtt.io/devices/TS0012.html) | [`AlEx`](https://www.aliexpress.com/item/1005003324697513.html) | [`#051`](https://github.com/romasku/tuya-zigbee-switch/issues/51) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_7aqaupa9`  | [BSEED 3-gang touch switch](https://www.zigbee2mqtt.io/devices/TS0003.html) | [`AlEx`](https://www.aliexpress.com/item/1005003475686409.html) | [`#125`](https://github.com/romasku/tuya-zigbee-switch/issues/125) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_zmy4lslw`  | [Girier 2-gang](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) | [`AlEx`](https://www.aliexpress.com/item/1005006084763437.html) | [`#029`](https://github.com/romasku/tuya-zigbee-switch/issues/29) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_ypgri8yz`  | [Girier ZB08](https://www.zigbee2mqtt.io/devices/ZB08.html) |   | [`#037`](https://github.com/romasku/tuya-zigbee-switch/issues/37) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_qq9ahj6z`  | [iHseno 1-gang touch switch](https://www.zigbee2mqtt.io/devices/_TZ3000_qq9ahj6z.html) | [`AlEx`](https://www.aliexpress.com/item/1005007532195287.html) | [`#146`](https://github.com/romasku/tuya-zigbee-switch/issues/146) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_zxrfobzw`  | [iHseno 2-gang touch switch](https://www.zigbee2mqtt.io/devices/_TZ3000_zxrfobzw.html) | [`AlEx`](https://www.aliexpress.com/item/1005007532195287.html) | [`#146`](https://github.com/romasku/tuya-zigbee-switch/issues/146) | Supported | 
| 🟧 | ❌ | 🔌 | `    unknown     `  | [iHseno 3-gang touch switch](https://www.zigbee2mqtt.io/devices/TS0003.html) | [`AlEx`](https://www.aliexpress.com/item/1005007532195287.html) | [`#146`](https://github.com/romasku/tuya-zigbee-switch/issues/146) | Needs IDs | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_pgq7ormg`  | [iHseno 1-gang](https://www.zigbee2mqtt.io/devices/_TZ3000_pgq7ormg.html) | [`AlEx`](https://www.aliexpress.com/item/1005008107698143.html) | [`#105`](https://github.com/romasku/tuya-zigbee-switch/issues/105) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_mhhxxjrs`  | [iHseno 3-gang](https://www.zigbee2mqtt.io/devices/TS0003.html) | [`AlEx`](https://www.aliexpress.com/item/1005008107698143.html) | [`#085`](https://github.com/romasku/tuya-zigbee-switch/issues/85) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_knoj8lpk`  | [iHseno 4-gang](https://www.zigbee2mqtt.io/devices/TS0004.html) | [`AlEx`](https://www.aliexpress.com/item/1005008107698143.html) | [`#105`](https://github.com/romasku/tuya-zigbee-switch/issues/105) | Supported | 
| 🟧 | ❌ | 🔌 | `_TZ3000_tqwydnqn`  | [Manhot 3-gang switch](https://www.zigbee2mqtt.io/devices/TS0013.html) | [`AlEx`](https://www.aliexpress.com/item/1005008300010160.html) [`AlEx`](https://www.aliexpress.com/item/1005008867890261.html) | [`#128`](https://github.com/romasku/tuya-zigbee-switch/issues/128) | Bi-stable relays not implemented + can not toggle multiple relays at once | 
| 🟨 | ✔️ | 🔌 | `_TZ3000_imaccztn`  | [MHCOZY TYWB 4ch-RF](https://www.zigbee2mqtt.io/devices/TYWB_4ch-RF.html) | [`AlEx`](https://www.aliexpress.com/item/1005005457296054.html) | [`#130`](https://github.com/romasku/tuya-zigbee-switch/issues/130) | Untesed? Unknown pins for reset button and blue LED? (C4, B4, B1) | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_hhiodade`  | [Moes 1-gang switches (button or touch)](https://www.zigbee2mqtt.io/devices/ZS-EUB_1gang.html) | [`AlEx`](https://www.aliexpress.com/item/1005005178075186.html) [`Moes`](https://moeshouse.com/collections/eu-star-ring) | [`#014`](https://github.com/romasku/tuya-zigbee-switch/issues/14) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_18ejxno0`  | [Moes 2-gang switches (buttons or touch)](https://www.zigbee2mqtt.io/devices/ZS-EUB_2gang.html) | [`AlEx`](https://www.aliexpress.com/item/1005005178075186.html) [`Moes`](https://moeshouse.com/collections/eu-star-ring) | [`#014`](https://github.com/romasku/tuya-zigbee-switch/issues/14) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_qewo8dlz`  | [Moes 3-gang switches (buttons or touch)](https://www.zigbee2mqtt.io/devices/TS0013.html) | [`AlEx`](https://www.aliexpress.com/item/1005005178075186.html) [`Moes`](https://moeshouse.com/collections/eu-star-ring) | [`#014`](https://github.com/romasku/tuya-zigbee-switch/issues/14) | Supported | 
| 🟨 | ✔️ | 🔌 | `_TZ3000_qaa59zqd`  | [Moes ZM-104B-M](https://www.zigbee2mqtt.io/devices/ZM-104B-M.html) | [`Amzn`](https://www.amazon.co.uk/dp/B0CKYP32CW?th=1) | [`#147`](https://github.com/romasku/tuya-zigbee-switch/pull/147) | Has buzzer instead of LED | 
| 🟨 | ✔️ | 🔌 | `_TZ3000_pfc7i3kt`  | [Moes MS-104CZ](https://www.zigbee2mqtt.io/devices/TS0003.html) | [`AlEx`](https://www.aliexpress.com/item/1005004986958450.html) | [`#030`](https://github.com/romasku/tuya-zigbee-switch/pull/30) | Has buzzer on C2 | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_bvrlqyj7`  | [OXT 2-gang](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) |   | [`#049`](https://github.com/romasku/tuya-zigbee-switch/issues/49) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_tqlv4ug4`  | [Tuya 1-gang 🅰](https://www.zigbee2mqtt.io/devices/TS0001_switch_module.html) |   | [`#006`](https://github.com/romasku/tuya-zigbee-switch/issues/6) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_skueekg3`  | [Tuya 1-gang 🅱](https://www.zigbee2mqtt.io/devices/WHD02.html) |   |   | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_skueekg3`  | [Tuya 1-gang 🅒](https://www.zigbee2mqtt.io/devices/WHD02.html) |   | [`#024`](https://github.com/romasku/tuya-zigbee-switch/issues/24) [`#080`](https://github.com/romasku/tuya-zigbee-switch/issues/80) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_skueekg3`  | [Tuya 1-gang 🅓](https://www.zigbee2mqtt.io/devices/WHD02.html) |   | [`#044`](https://github.com/romasku/tuya-zigbee-switch/issues/44) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_01gpyda5`  | [Tuya 2-gang](https://www.zigbee2mqtt.io/devices/TS0002_basic.html) |   | [`#006`](https://github.com/romasku/tuya-zigbee-switch/issues/6) | Supported | 
| 🟨 | ✔️ | 🔌 | `_TZ3000_ltt60asa`  | [Tuya 4-gang](https://www.zigbee2mqtt.io/devices/TS0004_switch_module.html) |   | [`#042`](https://github.com/romasku/tuya-zigbee-switch/issues/42) | Some issues with reporting and groups | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_ji4araar`  | [Tuya 1-gang L-only](https://www.zigbee2mqtt.io/devices/TS0011_switch_module.html) |   | [`#004`](https://github.com/romasku/tuya-zigbee-switch/issues/4) | Supported | 
| 🟩 | ✔️ | 🔌 | `_TZ3000_jl7qyupf`  | [Tuya 2-gang L-only](https://www.zigbee2mqtt.io/devices/TS0012_switch_module.html) |   |   | Supported | 
| 🟧 | ✔️ | 🔋 | `_TZ3000_mh9px7cq`  | [Tuya 4-button remote](https://www.zigbee2mqtt.io/devices/TS0044_1.html) | [`Store`](https://allegro.pl/oferta/pilot-atlo-rc4-tuya-zigbee-tuya-smart-da-16513965398) | [`#171`](https://github.com/romasku/tuya-zigbee-switch/issues/171) | Huge battery drain! Does not have relays. | 
| 🟨 | ✔️ | 🔌 | `_TZ3000_zmlunnhy`  | [Zemismart 2-gang switch](https://www.zigbee2mqtt.io/devices/TS0012.html) |   | [`#019`](https://github.com/romasku/tuya-zigbee-switch/issues/19) | Untested | 

Data from [`device_db.yaml`](/device_db.yaml)
