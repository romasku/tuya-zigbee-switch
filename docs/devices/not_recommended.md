# Not recommended devices
Please add devices that:
- do not use Telink chips (not supported)
- do not have the OTA cluster
- have coil whine (loud screeching noise)
- have [multiple_pinouts.md](./multiple_pinouts.md)
- have other issues

## Relays
#### `_TZ3000_skueekg3` - WHD02 (1-gang)
- There are multiple variants with the same ID.
- Some can not update OTA: [#80](https://github.com/romasku/tuya-zigbee-switch/issues/80)

#### `_TZ3000_5ajpkyq6` - AVATTO ZWSM16-4 (4-gang only)
- There are **2 known variants** with the same ID and image_type **that have different pinouts**.
- It should be okay if you get it from [AliExpress](https://vi.aliexpress.com/item/1005007247647375.html) (no affiliate). This is the default supported variant.  
Otherwise, check the pinout before flashing.
- Further reading: [multiple_pinouts.md](./multiple_pinouts.md)

## Switches