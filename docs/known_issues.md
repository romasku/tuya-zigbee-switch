# Open issues

These issues are not yet fixed and they affect every device.  
If you have devices on older versions, also read [# Closed issues](#closed-issues).

## Bi-stable relays draw too much power

Discussion: [#70](https://github.com/romasku/tuya-zigbee-switch/issues/70) + Discord  

Bi-stable relays are sometimes found in switches without Neutral.  
They require a pulse instead of constant power, **which is not implemented yet**.  
The higher power draw than stock firmware might **prevent the device from functioning properly**.  

## Device randomly freezes (until power-cycled)

Discussion: [#122](https://github.com/romasku/tuya-zigbee-switch/issues/122) + Discord  

There have been reports from 2 users occasionally finding their devices unresponsive.  
The devices recover after a reboot.

This is a routing issue that happens on big networks, when the custom device attempts routing some specific devices through it.

This issue was fixed in a newer SDK version, but we haven't migrated yet.

It can be avoided by disabling routing (using end_device firmware) or by trying out firmware built with the latest SDK.  

## Error message on boot

Discussion: [#40](https://github.com/romasku/tuya-zigbee-switch/issues/40)

The following error is expected every time the switch reboots and **can safely be ignored**:  
> Failed to read state of 'Switch' after reconnect  

It's a consequence of having individual endpoints for relays and switches (which are needed for binding both ways).  
Z2M attempts to read the relay state on the first endpoint, but we actually use it for sending commands.

## IKEA remote not binding to the device

Discussion: Discord (mishakov)

One user reported some issues when binding an IKEA remote to the custom device.  
The devices don't bind correctly and flood the network with broadcast messages.

This issue was possibly fixed in a newer SDK version, but we haven't migrated yet.

# Closed issues

The following issues were already fixed. They only affect devices on older versions.  
They are still mentioned here because **they cause bricking and prevent updates**.  

Issues that are no longer mentioned have been fixed and do not have negative consequences.

## Custom DEVICE CONFIG bricks unit (fixed in v18-20)

Discussion: [#77](https://github.com/romasku/tuya-zigbee-switch/issues/77)

> [!NOTE]  
> The **device config field** is an advanced option that allows changing the pre-defined *device config string* without updating the whole firmware.  
>  
> Currently, it is useful in debugging and supporting new devices.  
> **Do not touch it otherwise!** It applies as soon as you click away from the field.
>
> We are slowly moving towards a unified firmware without the config string pre-defined (where the user would use this field to set it per device). But it is not **yet** recommended to be used as a permanent solution.  
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

Note that it is still possible to brick your device by providing an incorrect config string.  
If you bricked your device, [flashing_via_wire.md](./flashing_via_wire.md) is the only recovery method.  

## 4-gang devices can't update OTA (fixed between v17 and v18)

**Up to version 17**, 4-gang devices could not update OTA.  
It is possible to enable updates by giving it a 3-gang config string.  
But the [previously mentioned issue](#custom-device-config-bricks-unit-fixed-in-v18-20) makes it harder. (**Do not try it yourself** without a UART flasher handy)  
Please open an issue if you are in this scenario and we will test a solution before giving you instructions.