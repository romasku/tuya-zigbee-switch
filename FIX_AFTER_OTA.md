# Fix Issues After OTA Update

## Problems Reported
1. Still seeing "right and left sides" (should only be 1 switch)
2. Right position isn't toggleable
3. Physical switches work only in momentary way (need to double switch)
4. Right switch option appears to be controlling the relay

## Solutions

### Step 1: Re-Interview Device

The device needs to be re-interviewed to pick up the new converter configuration:

1. **In Zigbee2MQTT:**
   - Go to your device page
   - Click the **`i`** button (Interview) to re-interview the device
   - Wait for interview to complete

2. **Or remove and re-add:**
   - Remove device from Z2M
   - Factory reset device (5-second long press on module button)
   - Re-add device to Z2M

### Step 2: Configure Switch Settings

After re-interview, configure the switch endpoint:

1. **Go to device settings in Z2M**

2. **For the switch endpoint, set:**
   - **Switch mode:** `toggle` (NOT `momentary`)
   - **Switch action mode:** `toggle_simple` or `toggle_smart_sync`
   - **Switch relay mode:** `short_press`
   - **Switch relay index:** `relay_1` (should be 1)
   - **Switch long press duration:** `800` (standard)

3. **For the relay endpoint:**
   - **State:** Should toggle when switch is pressed
   - **Power-on behavior:** `previous` (or your preference)

### Step 3: Understanding 2-Way Switch Behavior

A 2-way switch works differently than a regular toggle:
- **Position 1:** Circuit closed (S1 and S2 connected)
- **Position 2:** Circuit open (S1 and S2 disconnected)

The firmware detects the **state change** (closed → open or open → closed) and toggles the relay.

**Expected behavior:**
- Flip switch to position 1 → Relay toggles ON
- Flip switch to position 2 → Relay toggles OFF
- Each position change = one toggle

If you need to "double switch" it, the switch mode might be wrong.

### Step 4: If Still Not Working

If the switch still acts like momentary:

1. **Try `toggle_smart_sync` instead of `toggle_simple`:**
   - This keeps the switch position in sync with relay state
   - Better for 2-way switches

2. **Check if switch mode is actually `toggle`:**
   - In Z2M device settings, verify "Switch mode" is set to `toggle`
   - If it's `momentary`, change it to `toggle`

3. **Verify relay_index:**
   - Make sure "Switch relay index" is set to `relay_1` (value: 1)

4. **If you still see 2 switch endpoints:**
   - The converter wasn't regenerated properly
   - Need to regenerate converters and re-interview

## Quick Fix Checklist

- [ ] Re-interview device (click `i` button in Z2M)
- [ ] Verify only 1 switch endpoint appears (not 2)
- [ ] Set switch mode to `toggle`
- [ ] Set switch action mode to `toggle_simple` or `toggle_smart_sync`
- [ ] Set switch relay index to `relay_1`
- [ ] Test physical switch - should toggle relay on each position change

