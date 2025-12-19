# OTA Update Instructions for TUYA_MODULE_TS011F

## What We've Done

✅ Added your device (`_TZ3000_i9oy2rdq`, `TS011F`) to the device database as `TUYA_MODULE_TS011F`
✅ Configured it with button pin B4 (based on similar BSEED socket)
✅ Assigned firmware image type: 45629

## Next Steps: Build Firmware

You have two options to build the firmware:

### Option 1: GitHub Actions (Recommended - Easiest)

**⚠️ IMPORTANT: You need to fork the repository first!**

1. **Fork the repository:**
   - Go to: https://github.com/romasku/tuya-zigbee-switch
   - Click the "Fork" button (top right)
   - This creates a copy in your GitHub account

2. **Clone your fork locally:**
   ```bash
   git clone https://github.com/YOUR_USERNAME/tuya-zigbee-switch.git
   cd tuya-zigbee-switch
   ```

3. **Commit and push your changes:**
   ```bash
   git add device_db.yaml
   git commit -m "Add TUYA_MODULE_TS011F device entry"
   git push origin main
   ```

4. **Trigger GitHub Actions build:**
   - Go to: https://github.com/YOUR_USERNAME/tuya-zigbee-switch/actions
   - Click "Build Firmware" workflow
   - Click "Run workflow"
   - Select your branch (usually `main`)
   - Run the workflow

5. **Wait for build to complete** (usually 5-10 minutes)

6. **Download the firmware:**
   - Go to the completed workflow run
   - Download the artifacts
   - Extract `bin/router/TUYA_MODULE_TS011F/tlc_switch-*.zigbee` file

### Option 2: Build Locally (If you have build tools)

If you have the Telink toolchain installed:

```bash
# Install dependencies (first time only)
make_scripts/make_install.sh

# Build firmware
BOARD=TUYA_MODULE_TS011F make board/build
```

The firmware will be in: `bin/router/TUYA_MODULE_TS011F/tlc_switch-*.zigbee`

## OTA Update Process

### Step 1: Update OTA Index

After building, the OTA index should be automatically updated. 

**If building via GitHub Actions:**
1. **Download the updated index file:**
   - From GitHub Actions artifacts, get `zigbee2mqtt/ota/index_router.json`
   - Or use the FORCE index URL (replace YOUR_USERNAME with your GitHub username):
     ```
     https://raw.githubusercontent.com/YOUR_USERNAME/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_router-FORCE.json
     ```

**If building locally:**
- The index will be automatically updated in `zigbee2mqtt/ota/index_router.json`
- You can host it yourself or use the FORCE index from your fork

### Step 2: Configure Zigbee2MQTT

1. **Add OTA index to Z2M:**
   - Go to Z2M Settings → OTA updates
   - Set "OTA index override file name" to your index URL
   - Or add to `configuration.yaml`:
     ```yaml
     ota:
       zigbee_ota_override_index_location: >-
         https://raw.githubusercontent.com/YOUR_USERNAME/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_router-FORCE.json
     ```

2. **Restart Zigbee2MQTT**

### Step 3: Update Device

1. **Ensure strong signal** - bring device close to coordinator
2. **In Z2M device page**, click "Update" button
3. **Wait for update to complete** (may show 100% for a while - that's normal)
4. **After update:**
   - Device will blink LED (if configured correctly)
   - **Interview device** (click `i` button in Z2M)
   - **Reconfigure device** (click `🗘` button in Z2M)

### Step 4: Test Physical Button

After the update, test the physical button:
- ✅ If it works: Great! You're done.
- ❌ If it still doesn't work: The button pin might be wrong (see troubleshooting below)

## Troubleshooting: Button Still Doesn't Work

If the physical button still doesn't work after the update, the button pin configuration might be incorrect. 

### Try Alternative Button Pins

Edit `device_db.yaml` and change the button pin in the `config_str`:

**Current configuration:**
```yaml
config_str: i9oy2rdq;TS011F-TUYA;LC2;SB4u;RC3;ID2;M;
```
(Button is on pin B4: `SB4u`)

**Try these alternatives:**

1. **Button on B5** (like BSEED PM socket):
   ```yaml
   config_str: i9oy2rdq;TS011F-TUYA;LC2;SB5u;RC3;ID2;M;
   ```

2. **Button on B6**:
   ```yaml
   config_str: i9oy2rdq;TS011F-TUYA;LC2;SB6u;RC3;ID2;M;
   ```

3. **Button on B7**:
   ```yaml
   config_str: i9oy2rdq;TS011F-TUYA;LC2;SB7u;RC3;ID2;M;
   ```

4. **Button with pull-down instead of pull-up** (change `u` to `d`):
   ```yaml
   config_str: i9oy2rdq;TS011F-TUYA;LC2;SB4d;RC3;ID2;M;
   ```

After changing, rebuild and update again.

### Finding the Correct Button Pin

If none of the common pins work, you may need to:
1. Open the device and check the PCB
2. Look for button connections to GPIO pins
3. Or try extracting pinout from stock firmware (see `helper_scripts/extract_pinout_from_stock_fw.py`)

## Current Configuration Details

- **Device Key:** `TUYA_MODULE_TS011F`
- **Manufacturer ID:** `_TZ3000_i9oy2rdq`
- **Firmware Image Type:** `45629`
- **Config String:** `i9oy2rdq;TS011F-TUYA;LC2;SB4u;RC3;ID2;M;`
  - `LC2` = Network LED on pin C2
  - `SB4u` = Button on pin B4 with pull-up
  - `RC3` = Relay on pin C3
  - `ID2` = Indicator LED on pin D2
  - `M` = Momentary mode

## Notes

- The device is marked as `mostly_supported` because the button pin needs verification
- Remote triggers work (Zigbee commands), so relay and Zigbee stack are fine
- Only the physical button GPIO configuration needs to be correct
- Once the correct button pin is found, we can update the status to `fully_supported`

