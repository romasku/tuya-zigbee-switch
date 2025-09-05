# 🚨 Known issues

## Custom DEVICE CONFIG bricks unit (fixed in v18-20)

Discussion: [#77](https://github.com/romasku/tuya-zigbee-switch/issues/77)

> [!NOTE]  
> The **device config field** is an advanced option that allows changing the pre-defined *device config string* without updating the whole firmware.  
>  
> Currently, it is useful in debugging and supporting new devices.  
> **Do not touch it otherwise!**
>
> We are slowly moving towards a unified firmware without the config string pre-defined (where the user would use this field to set it per device). But it is not **yet** recommended to use it as a permanent solution.  
>
> If your device does not behave well on the default config string, please open an issue or suggest a change.

**Up to version 17**, interacting with the device config field:
- **immediately bricks 3-4 gang devices**
- possibly makes 1-2 gang devices unstable  

**If you touched the config string before v18** and your device survived:  
It is recommended you **reset the device and update to the latest version**.  
(Although, after testing, we concluded the devices were running fine even without resetting)  

**This was partly fixed in v18**, where the device would freeze and recover after a power-cycle.  
**The real bug was found and fixed in v20**, where it is safe to update the config string.

Note that it is still possible to brick your device by providing a GPIO pin that is not present on your device's Zigbee module. (This will be prevented in further updates)

If you bricked your device, [flashing_via_wire.md](./flashing_via_wire.md) will restore it.  

## 4-gang devices can't update OTA (fixed between v17 and v18)

**Up to version 17**, 4-gang devices can not update OTA.  
It is possible to enable updates by giving it a 3-gang config string,  
but the previously mentioned issue makes that harder.  (**Do not try it yourself** without a UART flasher handy)  
Please open an issue if you are in this scenario.

## Bi-stable relays draw too much power (not fixed yet)

Discussion: [#70](https://github.com/romasku/tuya-zigbee-switch/issues/70)  

Bi-stable relays are sometimes found in touch switches without Neutral.  
They relays require a pulse instead of constant power, **which is not implemented yet**.  
The higher power draw than stock firmware might **prevent the device from functioning properly**.  