# Known issues

## Custom DEVICE CONFIG bricks unit (fixed in v18)
Up to version 17, interacting with the device config field:
- immediately bricks 3-4 gang devices
- possibly makes 1-2 gang devices unstable  


If you touched the config string before v18, it is recommended you reset the device and update to the latest version. (Although it should be fine even without resetting)  
If you bricked your device.. [flash by wire](https://github.com/romasku/tuya-zigbee-switch/blob/main/docs/flashing_via_wire.md).  
Discussion: [#77](https://github.com/romasku/tuya-zigbee-switch/issues/77).

## 4-gang devices can't update OTA (fixed between v17 and v18)

Up to version 17, 4-gang devices can not update OTA.  
It is possible to enable updates by giving it a 3-gang config string, but the previously mentioned issue makes that harder. (Do not try it yourself)  
Please open an issue if you are in this scenario.

## Bi-stable relays draw too much power (not fixed yet)

Bi-stable relays require a pulse instead of constant power, which is not implemented yet.  
They are usually found on touch switches without Neutral.  
The high power draw than stock firmware might prevent the device from functioning properly.  
Discussion: [#70](https://github.com/romasku/tuya-zigbee-switch/issues/70).  