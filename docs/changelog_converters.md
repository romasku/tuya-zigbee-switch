# Custom Z2M Converters Changelog

Format: features.devices

## v1.1

### Changes:
- Updated state names **(breaking change)** 
  - e.g. `switch_left_feature_switch_left` → `feature_switch_left`
  - **Automations and templates that use device states will need updating.**
  - **The old (and unresponsive) names might still show up in Z2M / HA.**  
    Remove them from state history (`zigbee2mqtt/state.json`) and restart Z2M.  
    Deleting the file entirely is also safe. It will clear the last known state of all devices. 
  - Discussion: [#156](https://github.com/romasku/tuya-zigbee-switch/pull/156)