# Next Steps - Build and Update Your Socket

## ✅ What We've Done
- ✅ Added your device to `device_db.yaml`
- ✅ Updated git remote to your fork
- ✅ Committed and pushed changes to GitHub

## Step 1: Build Firmware via GitHub Actions

1. **Go to your repository:**
   https://github.com/BrunoMMendonca/tuya-zigbee-switch

2. **Click on "Actions" tab** (top menu)

3. **Click "Build Firmware" workflow** (on the left sidebar)

4. **Click "Run workflow"** (top right, next to "Filter" dropdown)

5. **Select branch:** `main` (should be default)

6. **Click the green "Run workflow" button**

7. **Wait for the build to complete** (usually 5-10 minutes)
   - You'll see a yellow dot while it's running
   - It will turn green when complete ✅

## Step 2: Download the Firmware

1. **Once the workflow completes** (green checkmark):
   - Click on the completed workflow run
   - Scroll down to "Artifacts" section
   - Click on the artifact (usually named something like "firmware" or "bin")

2. **Download and extract the zip file**

3. **Find your firmware file:**
   - Navigate to: `bin/router/TUYA_MODULE_TS011F/`
   - Look for: `tlc_switch-*.zigbee` file
   - Also get: `zigbee2mqtt/ota/index_router.json` (or `index_router-FORCE.json`)

## Step 3: Configure Zigbee2MQTT for OTA Update

You have two options:

### Option A: Use FORCE Index (Recommended - if device already has custom firmware)

1. **In Zigbee2MQTT Settings:**
   - Go to: **Settings → OTA updates**
   - Find: **"OTA index override file name"**
   - Enter:
     ```
     https://raw.githubusercontent.com/BrunoMMendonca/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_router-FORCE.json
     ```

2. **Or add to `configuration.yaml`:**
   ```yaml
   ota:
     zigbee_ota_override_index_location: >-
       https://raw.githubusercontent.com/BrunoMMendonca/tuya-zigbee-switch/refs/heads/main/zigbee2mqtt/ota/index_router-FORCE.json
   ```

3. **Restart Zigbee2MQTT**

### Option B: Manual OTA Update (If index doesn't work)

If the automatic OTA doesn't work, you can manually trigger it:

1. **Download the `.zigbee` file** from GitHub Actions artifacts
2. **Host it somewhere accessible** (or use GitHub raw URL)
3. **In Z2M device page**, use the manual OTA update option

## Step 4: Update Your Device

1. **Ensure strong signal:**
   - Bring the socket close to your Zigbee coordinator
   - Or ensure good signal strength (check link quality in Z2M)

2. **In Zigbee2MQTT:**
   - Go to your socket device page
   - Click the **"Update"** button (or "OTA update")
   - Wait for update to complete (may show 100% for a while - that's normal)

3. **After update completes:**
   - **Interview device** - Click the `i` button in Z2M device page
   - **Reconfigure device** - Click the `🗘` (refresh) button
   - This updates endpoints and clusters

## Step 5: Test the Physical Button

After the update, test the physical button on your socket:

- ✅ **If it works:** Great! You're done!
- ❌ **If it still doesn't work:** The button pin might be wrong (see troubleshooting below)

## Troubleshooting: Button Still Doesn't Work

If the physical button still doesn't work, we need to try a different button pin.

### Quick Fix - Try Alternative Button Pins

1. **Edit `device_db.yaml`** locally
2. **Find the `TUYA_MODULE_TS011F` entry**
3. **Change the button pin** in `config_str`:

**Current (B4):**
```yaml
config_str: i9oy2rdq;TS011F-TUYA;LC2;SB4u;RC3;ID2;M;
```

**Try these alternatives:**

1. **Button on B5:**
   ```yaml
   config_str: i9oy2rdq;TS011F-TUYA;LC2;SB5u;RC3;ID2;M;
   ```

2. **Button on B6:**
   ```yaml
   config_str: i9oy2rdq;TS011F-TUYA;LC2;SB6u;RC3;ID2;M;
   ```

3. **Button on B7:**
   ```yaml
   config_str: i9oy2rdq;TS011F-TUYA;LC2;SB7u;RC3;ID2;M;
   ```

4. **Button with pull-down (change `u` to `d`):**
   ```yaml
   config_str: i9oy2rdq;TS011F-TUYA;LC2;SB4d;RC3;ID2;M;
   ```

4. **After changing:**
   - Commit and push again
   - Rebuild via GitHub Actions
   - Update device again

## Need Help?

If you get stuck at any step, let me know:
- What step you're on
- Any error messages
- What's happening (or not happening)

I can help troubleshoot!

