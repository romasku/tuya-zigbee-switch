*Open the **Outline** (table of contents) from the top right.*  

# ⚠️ Open issues

*Issues affecting the **latest version***

### Bugs

- Switch randomly toggles on TLSR8253 512KB devices ([#289](https://github.com/romasku/tuya-zigbee-switch/issues/289))
  (HOBEIAN and Zbeacon)
- *Power-on behavior* doesn't fully work on some devices
- Telink Router sometimes unavailable? ([#255](https://github.com/romasku/tuya-zigbee-switch/issues/255))
- *momentary_nc* not working after power loss.  
  (Apply the setting again)

### Power draw

- The cusom firmware draws more current than the original
- As a result, the **minimum Load increases for *no-Neutral devices*!**  
  - estimated values: >4W for EndDevice, >8W for Router
  - smart bulbs not recommended (<1W when brighness=0)  
    ⤷ dummy load (capactior) may be required

### End_device does not respond to Zigbee group commands
- same issue on stock firmware (Zigbee limitation)
- work-around: use Home Assistant groups or Router firmware

### Failed to read state of 'Switch' after reconnect 

- error shows up every time the switch reboots
- **can safely be ignored**, doesn't mean anything  
- discussion: [#40](https://github.com/romasku/tuya-zigbee-switch/issues/40)

<br>

# ✅ Closed issues

*Some of the already already fixed issues, only present on **old fw versions**.*

### (< 1.1.3) Telink End_device unreachable from Z2M after a while
- **fixed in latest build** (force update if affected)
- work-around: set Z2M 'Availablility check' to 10 mins or use Router firmware
- discussion: [#217](https://github.com/romasku/tuya-zigbee-switch/issues/217)

### (= v1.1.0) Devices can't update OTA
- Keep this version or flash by wire

### (< v1.0.18) Custom config string bricks unit
- Interacting with the *device config* field **immediately bricks 3-4 gang devices**
- Recover: flash by wire

### (< v1.0.17) 4-gang devices can't update OTA 
- Ask for support or flash by wire