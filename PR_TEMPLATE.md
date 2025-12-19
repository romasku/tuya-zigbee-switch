# Add support for Tuya switch module TS011F (_TZ3000_i9oy2rdq)

**Device Key:** `TUYA_MODULE_TS011F`

## Device Information

- **Zigbee Manufacturer:** `_TZ3000_i9oy2rdq`
- **Zigbee Model:** `TS011F`
- **MCU:** TLSR8258 (Telink)
- **Module Type:** ZTU
- **Category:** Switch Module (1-gang 2-way switch)
- **Power:** Mains (L+N required)

## Pinout Configuration

- **2-way switch circuit:** S1→GND, S2→D2
  - Two physical wall switches control the same relay
  - When either switch toggles, it completes the circuit between S1 (GND) and S2 (D2)
- **Module button:** B5 (BB5u)
  - Long press (5 seconds): factory reset / Zigbee pairing mode
  - Hidden in Z2M (reset only, no switch endpoint)
- **Network LED:** B4 (LB4)
  - Blinks during pairing, off when connected
- **Wall switch:** D2 (SD2u)
  - Controls the relay (toggle mode)
- **Relay:** C3 (RC3)

## Testing Results

- ✅ Physical 2-way switch works correctly (toggles relay on each position change)
- ✅ 2-way switch functionality confirmed (both wall switches control the same relay)
- ✅ Module button (B5) triggers reset/pairing on 5-second long press
- ✅ Network LED (B4) works correctly (blinks during pairing, off when connected)
- ✅ Remote control via Zigbee2MQTT works correctly (relay toggles via Z2M)
- ✅ OTA update successful (from stock firmware using FORCE index)
- ✅ Device reconnects properly after update
- ✅ Switch mode: `toggle` with `toggle_smart_sync` action mode works perfectly

## Pinout Determination Method

Pinout was determined using multimeter continuity testing:
- Tested S1 and S2 terminals to identify GPIO connections
- Confirmed S1 connects to GND
- Confirmed S2 connects to D2
- Module button tested and confirmed on B5 (multimeter continuity test)
- Network LED tested and confirmed on B4
- Relay output confirmed on C3 (RC3) - tested multiple pins, RC3 works correctly

## Configuration Details

- **Config String:** `i9oy2rdq;TS011F-TUYA;BB5u;LB4;SD2u;RC3;`
- **Firmware Image Type:** 45629 (unique custom firmware identifier)
- **Stock Image Type:** 54179 (for initial OTA update from stock)
- **Status:** `fully_supported`

## Special Features

- **2-way switch support:** Both physical wall switches control the same relay
- **Extended long press:** Module button uses 5-second long press for reset (instead of standard 2 seconds) to prevent accidental resets during handling
- **Hidden module button:** Module button is configured as reset-only (B entry) and doesn't appear as a switch endpoint in Z2M, matching Zbeacon TS0001 pattern
- **Relay pin:** Uses RC3 (same as BSEED_SOCKET_TS011F), not the default RC0
- **Button pin:** B5 (determined via multimeter continuity testing)
- **LED pin:** B4 (determined via multimeter continuity testing)

## Notes

- Device is a 1-gang 2-way switch module (not a socket, despite TS011F model name)
- Module button (B5) is reset-only (hidden in Z2M, matching Zbeacon TS0001 behavior)
- Long press duration extended to 5 seconds for the module button to prevent accidental factory resets
- Wall switch (D2) controls the relay (RC3)

## Board Photos

Device and board photos are included in `docs/.images/`:
- `TUYA_1_gang_2_way_MODULE_TS011F_ZTU.jpg` - ZTU module view
- `TUYA_1_gang_2_way_MODULE_TS011F_board_front.jpg` - PCB front
- `TUYA_1_gang_2_way_MODULE_TS011F_device_front..jpg` - Device front
- `TUYA_1_gang_2_way_MODULE__board_back.jpg` - PCB back

These photos help verify pinout connections and help others identify the same device.

## Related

- Based on similar ZTU module configurations (e.g., AVATTO_TS0001)
- First switch module entry for TS011F model (other TS011F entries are sockets)

