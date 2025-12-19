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
- **Module button:** C3 (BC3u)
  - Long press (5 seconds): factory reset / Zigbee pairing mode
  - Hidden in Z2M (reset only, no switch endpoint)
- **Relay:** C3 (RC3)

## Testing Results

- ✅ Physical 2-way switch works correctly (toggles relay on each position change)
- ✅ 2-way switch functionality confirmed (both wall switches control the same relay)
- ✅ Module button (C3) triggers reset/pairing on 5-second long press
- ✅ Remote control via Zigbee2MQTT works correctly (relay toggles via Z2M)
- ✅ OTA update successful (from stock firmware using FORCE index)
- ✅ Device reconnects properly after update
- ✅ Switch mode: `toggle` with `toggle_smart_sync` action mode works perfectly

## Pinout Determination Method

Pinout was determined using multimeter continuity testing:
- Tested S1 and S2 terminals to identify GPIO connections
- Confirmed S1 connects to GND
- Confirmed S2 connects to D2
- Module button tested and confirmed on C3
- Relay output confirmed on C3 (RC3) - tested multiple pins, RC3 works correctly

## Configuration Details

- **Config String:** `i9oy2rdq;TS011F-TUYA;BC3u;SD2u;RC3;`
- **Firmware Image Type:** 45629 (unique custom firmware identifier)
- **Stock Image Type:** 54179 (for initial OTA update from stock)
- **Status:** `fully_supported`

## Special Features

- **2-way switch support:** Both physical wall switches control the same relay
- **Extended long press:** Module button uses 5-second long press for reset (instead of standard 2 seconds) to prevent accidental resets during handling
- **Hidden module button:** Module button is configured as reset-only (B entry) and doesn't appear as a switch endpoint in Z2M, matching Zbeacon TS0001 pattern
- **Relay pin:** Uses RC3 (same as BSEED_SOCKET_TS011F), not the default RC0

## Notes

- Device is a 1-gang 2-way switch module (not a socket, despite TS011F model name)
- The module button serves dual purpose: relay control + reset/pairing
- Long press duration extended to 5 seconds for the module button to prevent accidental factory resets
- Both switches (module button and 2-way wall switch) control the same relay (RC0)

## Board Photos (Optional but Helpful)

If you have photos of the PCB/board, please include them here. They help:
- Verify pinout connections
- Help others identify the same device
- Document the device for future reference

You can drag & drop images directly into this PR description.

## Related

- Based on similar ZTU module configurations (e.g., AVATTO_TS0001)
- First switch module entry for TS011F model (other TS011F entries are sockets)

