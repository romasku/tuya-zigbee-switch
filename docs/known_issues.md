*Open the **Outline** (table of contents) from the top right.*  

# ⚠️ Open issues

*Issues affecting the **latest version***

## Bi-stable relays draw too much power

- **will be fixed soon**
- discussion: [#207](https://github.com/romasku/tuya-zigbee-switch/pull/207), [#70](https://github.com/romasku/tuya-zigbee-switch/issues/70)

## Bugs

- ? Relay indicators sometimes go out-of-sync ([#38](https://github.com/romasku/tuya-zigbee-switch/issues/38))
- End_device sleeping? ([#217](https://github.com/romasku/tuya-zigbee-switch/issues/217))

## Failed to read state of 'Switch' after reconnect 

- error shows up every time the switch reboots
- **can safely be ignored**, doesn't mean anything  
- discussion: [#40](https://github.com/romasku/tuya-zigbee-switch/issues/40)

<br>

# ✅ Closed issues

*Issues already fixed, only present on **old fw versions**.*

## Custom config string bricks unit

- interacting with the 'device config' field **immediately bricks 3-4 gang devices**
- fixed in v18 (partly) and v20 (fully)
- recover: flash by wire
- discussion: [#77](https://github.com/romasku/tuya-zigbee-switch/issues/77)

## 4-gang devices can't update OTA

- fixed in v17-18
- ask for support or flash by wire