# Relay Pin Testing Plan

## Problem
Remote control doesn't work, suggesting the relay pin (RC0) might be incorrect.

## Current Configuration
- **Config:** `BC3u;SD2u;RC0;`
- **Relay Pin:** C0 (RC0)

## Alternative Relay Pins to Test

Based on other TS011F devices and common ZTU module patterns:

### Priority 1: RC3 (Most Common for TS011F)
- **BSEED_SOCKET_TS011F** uses: `RC3`
- **Config to test:** `BC3u;SD2u;RC3;`
- **Why:** Very common for TS011F devices

### Priority 2: RD2 (Common for ZTU modules)
- **Zbeacon TS0001** uses: `RD2`
- **Config to test:** `BC3u;SD2u;RD2;`
- **Why:** Common for ZTU switch modules

### Priority 3: RC2 (Also common)
- **Config to test:** `BC3u;SD2u;RC2;`
- **Why:** Some ZTU modules use C2 for relay

### Priority 4: RD3 (Less common but possible)
- **Config to test:** `BC3u;SD2u;RD3;`
- **Why:** Some devices use D3 for relay

## Testing Procedure

### Step 1: Build Firmware with Alternative Pin
1. Update `device_db.yaml` with new relay pin
2. Commit and push
3. Trigger GitHub Actions build
4. Wait for build to complete

### Step 2: Update Device via OTA
1. Download new firmware from GitHub Actions artifacts
2. Update OTA index (if needed)
3. Perform OTA update in Z2M
4. Wait for update to complete (~20 minutes)

### Step 3: Test Remote Control
1. Open Z2M device page
2. Try to toggle relay ON/OFF via Z2M interface
3. **Does the relay click?**
4. **Does the light/load turn on/off?**

### Step 4: Test Physical Buttons
1. Press wall switch
2. **Does relay toggle?**
3. Press module button (long press for reset)
4. **Does reset work?**

## Quick Test Order

Test in this order (most likely to least likely):

1. **RC3** - Most common for TS011F devices
2. **RD2** - Common for ZTU switch modules
3. **RC2** - Alternative C pin
4. **RD3** - Alternative D pin
5. **RC1** - Less common
6. **RD4** - Less common

## Success Criteria

✅ **Relay works via remote control** (Z2M toggle works)
✅ **Relay clicks when toggled**
✅ **Physical switch toggles relay**
✅ **Module button resets on 5s long press**

## Notes

- Each test requires a full firmware rebuild and OTA update (~20 min each)
- Keep track of which pins you've tested
- If none work, we may need to physically test the relay pin with a multimeter

