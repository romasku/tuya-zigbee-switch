# How to Flash via OTA  

**IMPORTANT**  
This process may brick your device, as it has not been extensively tested. Be prepared to use a hardware flasher to restore your device if needed. Although it worked fine for me, I want to warn you about the potential risks.  

To follow these instructions, you need **zigbee2mqtt** installed. If you're using **ZHA**, you'll need to do your own research (you can start [here](https://github.com/pvvx/ZigbeeTLc/issues/7)).  

### Step 1: Download the Necessary Files  
- [Converter for the original device](https://github.com/romasku/tuya-zigbee-switch/raw/refs/heads/main/zigbee2mqtt/converters_v1/tuya_with_ota.js)  
- [Converter for custom firmware](https://github.com/romasku/tuya-zigbee-switch/raw/refs/heads/main/zigbee2mqtt/converters_v1/switch_custom.js)  

- Custom index of OTA updates:
  * [If you want device to be Router](https://github.com/romasku/tuya-zigbee-switch/raw/refs/heads/main/zigbee2mqtt/ota/index_router.json)  
  * [If you want device to be End Device](https://github.com/romasku/tuya-zigbee-switch/raw/refs/heads/main/zigbee2mqtt/ota/index_end_device.json)  

Router device responds faster to requests, can increase Zigbee network strengs, but can be unstable as no neutral device and router firwmare consumes more power. You can try both options, but it requires [special OTA update](./change_device_type.md) to change type. 

Place all three files in your zigbee2mqtt data folder.  

### Step 2: Update the Configuration  

Add the following code to the `configuration.yaml` file of zigbee2mqtt:  

```yaml
external_converters:
  - switch_custom.js
  - tuya_with_ota.js
ota:
  zigbee_ota_override_index_location: !NAME_OF_DOWNLOADED_INDEX_FILE!
```

Replace `!NAME_OF_DOWNLOADED_INDEX_FILE!` with the name of the index file you downloaded (either `index_router.json` or `index_end_device.json`).

### Step 3: Verify the Configuration  

If everything is set up correctly, you should see something similar to this:  

![screen_ota_config](images/screen_ota_config.png)  

### Step 4: Flash via OTA

Restart zigbee2mqtt. Now device should appear in the OTA tab. Click "Check for new updates" and then proceed with the update.  


### Step 5: Rejoin the Device  

Once the device is flashed, force delete the old device from zigbee2mqtt and open your network by pressing "Permit join". Then reset the device it by pressing any switch 5 times in a row fast. The device should automatically rejoin.  

---

Hopefully, you now have a working device with custom firmware! 😊  