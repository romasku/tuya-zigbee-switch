# ZHA PWM Integration

ZHA quirk automatically detects PWM-capable devices and exposes PWM controls as Home Assistant entities. PWM support is only available on Router builds.

## Installation

1. Copy `zha/switch_quirk.py` to `config/custom_zha_quirks/switch_quirk.py`
2. Restart Home Assistant
3. Re-pair the device

## Entities

PWM-capable devices expose these additional entities:
- `switch.*_led_pwm_mode` - Enable/disable PWM
- `number.*_led_pwm_brightness` - PWM brightness (0-15)

## Technical Details

- **Cluster**: OnOff (0x0006) with custom attributes
- **PWM Enable**: 0xff03 (Boolean)
- **PWM Brightness**: 0xff04 (UInt8, range 0-15)

## Adding PWM to New Devices

Update device database:
```yaml
DEVICE_NAME:
  device_type: router  # PWM automatically enabled for router builds
```

Update ZHA quirk:
```python
PWM_CAPABLE_CONFIGS = {
    "Moes-2-gang",
    "NEW_DEVICE_MODEL",
}
```

## Limitations

- Router builds only (not End Device)
- 16 brightness levels (0-15)
- Requires line+neutral power