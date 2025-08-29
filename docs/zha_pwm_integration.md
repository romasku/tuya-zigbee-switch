# ZHA PWM Integration

This document describes how to use PWM LED dimming with Home Assistant's ZHA (Zigbee Home Automation) integration.

## Overview

The ZHA quirk automatically detects PWM-capable devices and exposes PWM controls as Home Assistant entities. PWM support is only available on Router builds with PWM capability configured in the device database.

## Supported Devices

### PWM-Capable Devices
- **Moes ZS-EUB 2-gang Switch** (Router variant)
  - ZHA Model: `Moes-2-gang`
  - PWM Controls: Available for both relay endpoints
  - End Device variant (`Moes-2-gang-ED`): PWM controls not exposed

## Home Assistant Entities

When a PWM-capable device is paired with ZHA, the following entities are automatically created:

### Standard Relay Controls
- `switch.moes_switch_relay_left` - Left relay on/off
- `switch.moes_switch_relay_right` - Right relay on/off

### LED Indicator Controls
- `select.moes_switch_relay_left_led_mode` - LED mode (same/opposite/manual)
- `switch.moes_switch_relay_left_led_state` - LED on/off state
- `select.moes_switch_relay_right_led_mode` - LED mode (same/opposite/manual)
- `switch.moes_switch_relay_right_led_state` - LED on/off state

### PWM Controls (Router builds only)
- `switch.moes_switch_relay_left_led_pwm_mode` - Enable/disable PWM for left LED
- `number.moes_switch_relay_left_led_pwm_brightness` - PWM brightness (0-15) for left LED
- `switch.moes_switch_relay_right_led_pwm_mode` - Enable/disable PWM for right LED
- `number.moes_switch_relay_right_led_pwm_brightness` - PWM brightness (0-15) for right LED

## Usage Examples

### Enable PWM with Medium Brightness
```yaml
service: switch.turn_on
target:
  entity_id: switch.moes_switch_relay_left_led_pwm_mode
---
service: number.set_value
target:
  entity_id: number.moes_switch_relay_left_led_pwm_brightness
data:
  value: 8
```

### Disable PWM (Use Normal LED Control)
```yaml
service: switch.turn_off
target:
  entity_id: switch.moes_switch_relay_left_led_pwm_mode
```

### Set Dim Night Mode
```yaml
service: switch.turn_on
target:
  entity_id: 
    - switch.moes_switch_relay_left_led_pwm_mode
    - switch.moes_switch_relay_right_led_pwm_mode
---
service: number.set_value
target:
  entity_id:
    - number.moes_switch_relay_left_led_pwm_brightness
    - number.moes_switch_relay_right_led_pwm_brightness
data:
  value: 2
```

## Automations

### Automatic Night Dimming
```yaml
automation:
  - alias: "Dim switch LEDs at sunset"
    trigger:
      - platform: sun
        event: sunset
    action:
      - service: switch.turn_on
        target:
          entity_id:
            - switch.moes_switch_relay_left_led_pwm_mode
            - switch.moes_switch_relay_right_led_pwm_mode
      - service: number.set_value
        target:
          entity_id:
            - number.moes_switch_relay_left_led_pwm_brightness
            - number.moes_switch_relay_right_led_pwm_brightness
        data:
          value: 3
```

### Bright LEDs During Day
```yaml
automation:
  - alias: "Bright switch LEDs at sunrise"
    trigger:
      - platform: sun
        event: sunrise
    action:
      - service: number.set_value
        target:
          entity_id:
            - number.moes_switch_relay_left_led_pwm_brightness
            - number.moes_switch_relay_right_led_pwm_brightness
        data:
          value: 12
```

### Motion-Based LED Dimming
```yaml
automation:
  - alias: "Dim LEDs when no motion"
    trigger:
      - platform: state
        entity_id: binary_sensor.motion_sensor
        to: 'off'
        for: '00:05:00'
    action:
      - service: number.set_value
        target:
          entity_id:
            - number.moes_switch_relay_left_led_pwm_brightness
            - number.moes_switch_relay_right_led_pwm_brightness
        data:
          value: 1
  
  - alias: "Bright LEDs when motion detected"
    trigger:
      - platform: state
        entity_id: binary_sensor.motion_sensor
        to: 'on'
    action:
      - service: number.set_value
        target:
          entity_id:
            - number.moes_switch_relay_left_led_pwm_brightness
            - number.moes_switch_relay_right_led_pwm_brightness
        data:
          value: 10
```

## Installation

### 1. Install the Quirk

Copy the `zha/switch_quirk.py` file to your Home Assistant custom quirks directory:

```
config/custom_zha_quirks/switch_quirk.py
```

### 2. Restart Home Assistant

Restart Home Assistant to load the new quirk.

### 3. Re-pair the Device

Remove and re-pair your Moes switch to apply the new quirk.

### 4. Verify PWM Support

Check that PWM entities appear in Home Assistant:
- Go to Settings → Devices & Services → ZHA
- Find your Moes switch device
- Verify PWM entities are listed

## Technical Details

### ZHA Quirk Implementation
- **Cluster**: OnOff (0x0006) with custom attributes
- **PWM Enable Attribute**: 0xff03 (Boolean)
- **PWM Brightness Attribute**: 0xff04 (UInt8, range 0-15)
- **Manufacturer Code**: Not required (standard attributes)

### Device Detection
The quirk automatically detects PWM capability:
- PWM controls are only exposed for devices in `PWM_CAPABLE_CONFIGS`
- End Device variants never expose PWM controls
- Router variants expose PWM controls if configured

### Entity Types
- **Switch entities**: PWM enable/disable controls
- **Number entities**: PWM brightness controls (0-15 range)
- **Select entities**: LED mode selection (same/opposite/manual)

## Troubleshooting

### PWM Entities Not Visible
1. **Check Device Type**: Ensure device is Router build (not End Device)
2. **Verify Quirk**: Confirm quirk is installed and loaded
3. **Re-pair Device**: Remove and re-pair the device
4. **Check Logs**: Look for ZHA errors in Home Assistant logs

### PWM Not Working
1. **Enable PWM**: Turn on the PWM mode switch first
2. **Set Brightness**: Ensure brightness is set above 0
3. **Check Firmware**: Verify device has PWM-enabled firmware
4. **Power Supply**: Ensure stable power to the device

### Entities Have Wrong Names
1. **Device Naming**: Rename the device in ZHA settings
2. **Entity Customization**: Customize entity names in Home Assistant
3. **Area Assignment**: Assign device to appropriate area

## Adding PWM to New Devices

To add PWM support to additional devices:

1. **Update Device Database**:
```yaml
DEVICE_NAME:
  device_type: router
  indicator_pwm: true
  default_indicator_brightness: 2
  pwm_capable_pins: [D3, C0]
```

2. **Update ZHA Quirk**:
```python
PWM_CAPABLE_CONFIGS = {
    "Moes-2-gang",
    "NEW_DEVICE_MODEL",  # Add new device model
}
```

3. **Test Integration**:
- Flash Router firmware with PWM support
- Install updated quirk
- Pair device and verify PWM entities appear

## Limitations

- PWM is only available on Router builds (not End Device)
- Maximum 16 brightness levels (0-15)
- Requires line+neutral power (no battery devices)
- Device must have PWM capability in firmware

## See Also

- [PWM LED Dimming](pwm_led_dimming.md) - General PWM documentation
- [Device Database Explained](device_db_explained.md) - Device configuration
- [Supported Devices](supported_devices.md) - List of all supported devices