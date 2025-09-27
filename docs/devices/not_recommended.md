# Not recommended devices

This page lists devices that you should avoid or be careful with.  
Open the **Outline** (table of contents) from the top right.  
Also check out: [recommended.md](./recommended.md)

## Coordinators

### zStack 
The adapters based on CC Texas Instruments chips are **unreliable on large networks**:  
- **SONOFF ZBDongle-P**
- **SMLIGHT SLZB-06**
- etc  

Most of our error reports come from users with these dongles.  

## Custom firmware candidates

**Devices that are the scope of this project** and are currently available for purchase should be on this list if they:
- do not use Telink chips (not supported) or
- do not have the OTA cluster (can't update OTA) or
- have coil whine (loud screeching noise) or
- have [multiple_pinouts.md](./multiple_pinouts.md) or
- have other issues

Older switches (around 2020?) use chips from Texas Instruments, Silicon Laboratories and are not compatible.

### Switch modules

#### WHD02 (1-gang) - `_TZ3000_skueekg3`
- There are multiple variants with the same ID.
- Some can not update OTA: [#80](https://github.com/romasku/tuya-zigbee-switch/issues/80)

#### AVATTO ZWSM16-4 (4-gang only) - `_TZ3000_5ajpkyq6`
- There are **2 known variants** with the same identifiers **that have different pinouts**.
- It should be okay if you get it from [AliExpress](https://vi.aliexpress.com/item/1005007247647375.html) (no affiliate). This is the default supported variant.  
Otherwise, check the pinout before flashing.
- Further reading: [multiple_pinouts.md](./multiple_pinouts.md)

### Switches

#### BSEED TS0726_2_gang - `_TZ3002_zjuvw9zf`
- The 2-gang BSEED switch has a loud coil whine (especially when 1 relay is on)
- Device discussion: [#157](https://github.com/romasku/tuya-zigbee-switch/issues/157)

### Sockets

## Other devices (original firmware)

**Devices that would interact with the custom switches** and are currently available for purchase should be on this list if they:
- spam the network (check the Health tab in Z2M)
- don't respond to binds correctly (on, off, toggle, dim up/down)
- act strange when the coordinator is offline
- are missing expected features
- have other issues

### Lightbulbs

### Lights