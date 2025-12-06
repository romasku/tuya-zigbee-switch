*Open the **Outline** (table of contents) from the top right.*  

# Firmware Changelog

<!------------------------------------------------------

Please describe what you are working on:


### Changes

- Bi-stable (latching) relays have been reworked and now use proper pulses instead of continuously driving the coil.


------------------------------------------------------->

## v1.1.2

*Bug-fix update*

### Bugs

- **Fixed** 
  - Setting 'long press duration' to 0ms crashes device
  - Can't change device imageType in config string
  - Option 'Relay indicator - manual on' is not kept after reboot
  - Relay indicators sometimes go out-of-sync ([#38](https://github.com/romasku/tuya-zigbee-switch/issues/38))

## v1.1.1

*Critical bug-fix update for previous version*

### Changes

- Added a **hardware watchdog** on Telink devices that automatically reboots the device when it's stuck

### Bugs

- **Fixed:** Floating pin (or other device conditions) freezes the device

## v1.1.0

*Contains **substantial restructuring** of the firmware architecture, but doesn't bring new features*

### Changes

- Updated **Telink SDK** to v3.7.2.0 for **stability**
- Added support for **Silabs chips** by introducing **HAL middleware**
- Replaced GPIO polling with GPIO interrupts for **power efficiency**
- Improve **versioning** to include commit hash
- Many more technical improvements

### Bugs

- **Fixed** 
  - Old SDK freezes device in some network conditions
- **New** 
  - Floating pin (or other device conditions) **freezes device**
  - Setting 'long press duration' to 0ms **crashes device**
  - Can't change device imageType in config string
  - Option 'Relay indicator - manual on' is not kept after reboot
  - Relay indicators sometimes go out-of-sync ([#38](https://github.com/romasku/tuya-zigbee-switch/issues/38))
  - End_device sleeping? ([#217](https://github.com/romasku/tuya-zigbee-switch/issues/217))

## v1.0.21

### Changes

- Keep device configuration (user settings) when it is removed from the network

### New features

- Add support for Zigbee commands: 
  - **off_with_effect** (0x40)
  - **on_with_recall_global_scene** (0x41)
- Add support for **normally-closed momentary buttons**
- Add **action states for toggle buttons**: position_on and position_off

## v1.0.20

### Changes

- (technical) Updated memory map: moved NV items from ZCL to APP.  
  **Due to this change, device configuration (user settings) may reset after OTA update.**

### Bugs

- **Fixed**
  - Canging config string crashed 3-4 gang devices
  - Detached mode didn't work for Toggle switches

## v1.0.19

### New features

- Add support for the **levelCtrl** cluster  
  - This enables brightness control of compatible Zigbee bulbs via Zigbee binding.  
  - The feature works only for momentary switches using long press: once a long press is detected, brightness will begin to slowly change. Each subsequent long press reverses the direction (increase/decrease).  
  - Requires manual update of converters and reconfiguration.

### Changes

- Increase the number of **presses required to reset the device to 10.**
- Update manufacturer names to match the stock firmware.  
  (requires interview; but it's not mandatory, as backwards compatibility is kept)

### Bugs

- New bug: detached mode doesn't work for Toggle switches

## v1.0.18

- Partly fix an issue where setting the config string could brick the device.
- Technical: introduce a method to update data stored in NVRAM in new releases.

## v1.0.17

- Fix once again power on behavior = OFF not working if toggle in pressed state during boot.

## v1.0.16

- Add new toggle modes: TOGGLE_SMART_SYNC/TOGGLE_SMART_OPPOSITE (requires re-download of `switch_custom.js`).

## v1.0.15

- Add support for Zigbee groups. Read [doc](/docs/usage/endpoints.md) for details about endpoints.

## v1.0.14

- Improve code logic for Indicator LED on for switches.

## v1.0.13

- Fix power on behavior = OFF not working if toggle in pressed state during boot.
- Add way to control network state led state (requires re-download of `switch_custom.js`).

## v1.0.12

- Fix led indicator state in manual mode not preserved after reboot.
- Add forced device announcement after boot to make sure device is seen as "available" as soon as it boots.
- Restored device pictures in z2m (requires re-download of `switch_custom.js`).
- Cleaned-up z2m converter (fix typos, inconsistent names, etc.). **Warning!** This may break your automations as it changes .
  property names (requires re-download of `switch_custom.js`).

## v1.0.11

- Improve join behaviour by decreasing timeout between tries to join.
- Fix leave network: now device will send LeaveNetwork command properly.
- Display firmware version in a human-readable form.

## v1.0.10

- Add support for bi-stable relays controlled by 2 pins.
- Fix Led indicator mode not preserved after reboot.

## v1.0.9

- Fix reporting of indicator led status.

## v1.0.8

- Add support for indicator leds.
- Add way to force momentary mode as default via config.

## v1.0.7

- Add SUSPEND-based sleep to EndDevice firmware to decrease power usage ~10x.

## v1.0.6

- Add way to change device pinout on the fly, to allow easier porting of firmware .

## v1.0.5

- Keep status LED on when device is connected.
- Add separate firmwares for End Device/Router.
- Improve device boot time significantly by removing unnecessary logs .

## v1.0.4

- Fix bug that caused report to be sent every second.

## v1.0.3

- Add support of startup behaviour: ON, OFF, TOGGLE, PREVIOUS.
- Add support of button actions: 'released', 'press', 'long_press'. This is only useful for momentary (doorbell-like) switches.

## v1.0.2

- Add way to reset the device by pressing any switch button 5 times in a row .
- Fix support for ON_OFF, OFF_ON actions.
