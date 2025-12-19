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
- **Module button:** C3 (SC3u)
  - Short press: toggles relay
  - Long press (5 seconds): factory reset / Zigbee pairing mode
- **Relay:** C0 (RC0)

## Testing Results

- ✅ Physical buttons work correctly
- ✅ 2-way switch functionality confirmed (both wall switches control the same relay)
- ✅ Module button (C3) toggles relay on short press
- ✅ Module button (C3) triggers reset/pairing on 5-second long press
- ✅ OTA update successful (from stock firmware)
- ✅ Remote control via Zigbee2MQTT works correctly
- ✅ Device reconnects properly after update

## Pinout Determination Method

Pinout was determined using multimeter continuity testing:
- Tested S1 and S2 terminals to identify GPIO connections
- Confirmed S1 connects to GND
- Confirmed S2 connects to D2
- Module button tested and confirmed on C3
- Relay output confirmed on C0

## Configuration Details

- **Config String:** `i9oy2rdq;TS011F-TUYA;SC3u;SD2u;RC0;`
- **Firmware Image Type:** 45629 (unique custom firmware identifier)
- **Stock Image Type:** 54179 (for initial OTA update from stock)
- **Status:** `fully_supported`

## Special Features

- **2-way switch support:** Both physical wall switches control the same relay
- **Extended long press:** Module button uses 5-second long press for reset (instead of standard 800ms) to prevent accidental resets
- **Module button functionality:** The on-board button both controls the relay AND provides reset functionality

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

