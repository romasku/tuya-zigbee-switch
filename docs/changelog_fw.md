# Firmware Changelog

### Upcoming:

- 

### v1.0.18

- Fix an issue where setting the config string could brick the device.  
- Technical: introduce a method to update data stored in NVRAM in new releases.

### v1.0.17

- Fix once again power on behavior = OFF not working if toggle in pressed state during boot.

### v1.0.16

- Add new toggle modes: TOGGLE_SMART_SYNC/TOGGLE_SMART_OPPOSITE (requires re-download of `switch_custom.js`).

### v1.0.15

- Add support for Zigbee groups. Read [doc](./docs/endpoints.md) for details about endpoints.

### v1.0.14

- Improve code logic for Indicator LED on for switches.

### v1.0.13

- Fix power on behavior = OFF not working if toggle in pressed state during boot.
- Add way to control network state led state (requires re-download of `switch_custom.js`).

### v1.0.12

- Fix led indicator state in manual mode not preserved after reboot.
- Add forced device announcement after boot to make sure device is seen as "available" as soon as it boots.
- Restored device pictures in z2m (requires re-download of `switch_custom.js`).
- Cleaned-up z2m converter (fix typos, inconsistent names, etc). **Warning!** This may break your automations as it changes .
  property names (requires re-download of `switch_custom.js`).

### v1.0.11

- Improve join behaviour by decreasing timeout between tries to join.
- Fix leave network: now device will send LeaveNetwork command properly.
- Display firmware version in a human-readable form.

### v1.0.10

- Add support for bi-stable relays contolled by 2 pins.
- Fix Led indicator mode not preserved after reboot.

### v1.0.9

- Fix reporting of indicator led status.

### v1.0.8

- Add support for indicator leds.
- Add way to force momentary mode as default via config.

### v1.0.7

- Add SUSPEND-based sleep to EndDevice firmware to decrease power usage ~10x.

### v1.0.6

- Add way to change device pinout on the fly, to allow easier porting of firmware .

### v1.0.5

- Keep status LED on when device is connected.
- Add separate firmwares for End Device/Router.
- Improve device boot time significantly by removing unnecessary logs .

### v1.0.4

- Fix bug that caused report to be sent every second.

### v1.0.3

- Add support of statup behaviour: ON, OFF, TOGGLE, PREVIOUS.
- Add support of button actions: 'released', 'press', 'long_press'. This is only useful for momentary (doorbell-like) switches.

### v1.0.2

- Add way to reset the device by pressing any switch button 5 times in a row .
- Fix support for ON_OFF, OFF_ON actions.
