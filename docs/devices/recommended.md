# Recommended devices

This page lists devices that are fully supported and have been tested by our community.  
Open the **Outline** (table of contents) from the top right.  
Also check out: [not_recommended.md](./not_recommended.md)

## Coordinators

### EmberZNet 
The adapters based on EFR32 Silicon Labs chips seem to be **the most reliable**, even on networks of 150+ devices:  
- **SONOFF ZBDongle-E**
- **SMLIGHT SLZB-06M**
- etc

Everybody who switched to Ember feels better about their setup. 

## Custom firmware candidates

**Devices that are the scope of this project** and are currently available for purchase should be on this list if they:
- use Telink chips (supported)
- have the OTA cluster (can update OTA)
- don't have coil whine (loud screeching noise)
- don't have [multiple_pinouts.md](./multiple_pinouts.md)
- don't have other issues

Most of the switches currently for sale on AliExpress use Telink chips and are compatible.  

### Switch modules

#### Avatto ZWSM16
- with Neutral, 1-4 relays with 1-4 switches, network LED, reset button, DIN rail mounting, supports OTA, no coil whine
- Get it from [AliExpress](https://vi.aliexpress.com/item/1005007247647375.html) (no affiliate). Be careful with the 4-gang from other stores: [multiple_pinouts.md](./multiple_pinouts.md)

#### Avatto LZWSM16
- without Neutral, 1-3 relays with 1-3 switches, network LED, reset button, DIN rail mounting, supports OTA, no coil whine?
- [AliExpress](https://vi.aliexpress.com/item/1005007247647375.html)

#### Aubess modules
- with Neutral, 1-4 relays with 1-4 switches, network LED, reset button, supports OTA, no coil whine
- [AliExpress](https://www.aliexpress.com/item/1005005748264739.html)

#### iHseno modules
- with Neutral, 1-4 relays with 1-4 switches, network LED, reset button, DIN rail mounting, supports OTA, no coil whine
- [AliExpress](https://www.aliexpress.com/item/1005008107698143.html)

### Switches

### Sockets

## Other devices (original firmware)

**Devices that would interact with the custom switches** and are currently available for purchase should be on this list if they:
- don't spam the network (check the Health tab in Z2M)
- respond to binds correctly (on, off, toggle, dim up/down)
- act normally even when the coordinator is offline
- have all expected features
- don't have other issues

### Lightbulbs

### Lights