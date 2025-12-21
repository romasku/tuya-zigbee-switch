*Open the **Outline** (table of contents) from the top right.*  

# ⚠️ Open issues

*Issues affecting the **latest version***

### Bugs

- Telink Router sometimes unavailable? ([#255](https://github.com/romasku/tuya-zigbee-switch/issues/255))

### Telink End_device unreachable from Z2M after a while
- **should be fixed in latest build**
- work-around: set Z2M 'Availablility check' to 10 mins or use Router firmware
- discussion: [#217](https://github.com/romasku/tuya-zigbee-switch/issues/217)

### End_device does not respond to Zigbee group commands
- same issue on stock firmware
- could be a Zigbee limitation
- work-around: use Home Assistant groups or Router firmware

### Failed to read state of 'Switch' after reconnect 

- error shows up every time the switch reboots
- **can safely be ignored**, doesn't mean anything  
- discussion: [#40](https://github.com/romasku/tuya-zigbee-switch/issues/40)

<br>

# ✅ Closed issues

*Some of the already already fixed issues, only present on **old fw versions**.*

### Custom config string bricks unit

- interacting with the 'device config' field **immediately bricks 3-4 gang devices**
- fixed in v18 (partly) and v20 (fully)
- recover: flash by wire
- discussion: [#77](https://github.com/romasku/tuya-zigbee-switch/issues/77)

### 4-gang devices can't update OTA

- fixed in v17-18
- ask for support or flash by wire