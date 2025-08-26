# PWM LED Dimming

This document describes the PWM (Pulse Width Modulation) LED dimming feature for indicator LEDs on supported devices.

## Overview

PWM support allows fine-grained control of indicator LED brightness on Router builds. This feature provides 16 discrete brightness levels (0-15) for smooth LED dimming.

**Important**: PWM is only available on Router builds with line+neutral power. End Device builds do not support PWM to preserve battery life.

## Supported Devices

### Router Builds with PWM Support
- **Moes ZS-EUB 2-gang Switch** (Router variant)
  - Device database key: `MOES_2_GANG_SWITCH`
  - PWM capable pins: D3, C0
  - Default brightness: 2 (~12.5%)

### End Device Builds
- All End Device variants do not support PWM
- PWM controls are automatically hidden in Z2M/ZHA

## Device Database Configuration

PWM support is configured in `device_db.yaml`:

```yaml
MOES_2_GANG_SWITCH:
  # ... existing fields ...
  # PWM fields (Router builds only)
  indicator_pwm: true                    # Enable PWM capability
  default_indicator_brightness: 2        # Default dimmed brightness (0-15)
  pwm_capable_pins: [D3, C0]            # Pins that support PWM dimming
```

## Zigbee2MQTT Integration

### Available Controls (Router builds only)
- `relay_left_indicator_pwm_mode` - Enable/disable PWM for left relay LED
- `relay_left_indicator_pwm_brightness` - Brightness level (0-15) for left relay LED
- `relay_right_indicator_pwm_mode` - Enable/disable PWM for right relay LED  
- `relay_right_indicator_pwm_brightness` - Brightness level (0-15) for right relay LED

### Home Assistant Entities
- Switch entities: `switch.device_relay_left_indicator_pwm_mode`
- Number entities: `number.device_relay_left_indicator_pwm_brightness`

## Usage Examples

### Enable PWM with Medium Brightness
```json
{
  "relay_left_indicator_pwm_mode": "ON",
  "relay_left_indicator_pwm_brightness": 8
}
```

### Night Mode (Dim LEDs)
```json
{
  "relay_left_indicator_pwm_mode": "ON",
  "relay_left_indicator_pwm_brightness": 2
}
```

### Disable PWM (Normal LED Control)
```json
{
  "relay_left_indicator_pwm_mode": "OFF"
}
```

## Technical Details

### Zigbee Attributes
- **Cluster**: genOnOff (0x0006)
- **PWM Enable**: Attribute 0xff03 (Boolean)
- **PWM Brightness**: Attribute 0xff04 (UInt8, range 0-15)
- **Manufacturer Code**: 0x0000

### PWM Implementation
- **Base Frequency**: 500Hz
- **Resolution**: 16 levels (4-bit)
- **ISR Rate**: Up to 8kHz for smooth dimming
- **Special Cases**: Brightness 0 and 15 bypass PWM for efficiency

### Feature Detection
Z2M/ZHA converters automatically detect PWM support:
1. Attempt to read PWM attributes during device configuration
2. If successful, expose PWM controls
3. If failed, hide PWM controls (graceful degradation)

## Troubleshooting

### PWM Controls Not Visible
- Ensure device is Router build (not End Device)
- Verify firmware has PWM support enabled
- Check device database has `indicator_pwm: true`
- Try re-pairing the device

### PWM Not Working
- Enable PWM mode first
- Set brightness above 0
- Check Zigbee logs for attribute errors
- Verify stable power supply

### LED Flickering
- Reduce brightness level
- Disable PWM to use normal LED control
- Check for power supply issues

## Adding PWM to New Devices

1. **Update Device Database**:
```yaml
DEVICE_NAME:
  device_type: router  # PWM only on Router builds
  indicator_pwm: true
  default_indicator_brightness: 2
  pwm_capable_pins: [D3, C0]  # Match your device pinout
```

2. **Update Z2M Converter**:
```javascript
// Add to device extend array
romasku.relayIndicatorPwmMode("relay_indicator_pwm_mode", "relay_endpoint"),
romasku.relayIndicatorPwmBrightness("relay_indicator_pwm_brightness", "relay_endpoint"),
```

3. **Test PWM Detection**:
- Flash Router firmware with PWM support
- Pair device and check Z2M logs for PWM detection
- Verify PWM controls appear in Home Assistant

## Firmware Compatibility

| Build Type | PWM Support | Notes |
|------------|-------------|-------|
| Router | ✅ Available | Full PWM functionality |
| End Device | ❌ Not available | Controls hidden automatically |

## See Also

- [Device Database Explained](device_db_explained.md) - Device configuration details
- [Supported Devices](supported_devices.md) - List of all supported devices
- [Project Structure](project_structure.md) - Codebase organization