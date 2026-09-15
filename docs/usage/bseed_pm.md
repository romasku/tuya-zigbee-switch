# BSEED TS011F power monitoring

`OUTLET_BSEED_PM_TS011F` (`_TZ3000_b28wrpvx`, model
`TS011F-BS-PM`) uses a BL0937-compatible pulse meter:

- CF: PA1
- CF1: PC2
- SEL: PB1, board-validated polarity

The legacy device configuration remains exactly
`b28wrpvx;TS011F-BS-PM;LC3;SB5u;RD2;IB4;M;`. Firmware selects the meter from
the manufacturer/model identity, so existing devices do not need a
`device_config` write.

The board defaults in `device_db.yaml` are hardware-revision defaults. A
device's calibration attributes persist finer per-unit calibration in NVM and
override those defaults. Do not copy calibration values between different
hardware revisions.

The firmware reports voltage (V), current (A), active power (W), apparent
power (VA), reactive-power magnitude (var), total power factor (%), and
accumulated energy (kWh) through standard Zigbee clusters. Confirmed no-load
readings suppress current, power, and energy accumulation after three samples;
voltage remains live and a real load exits suppression on its first sample.

Overload protection provides configurable sustained power/current limits,
trip delay, voltage warnings, reconnect delay, an immediate peak trip, and a
five-retry lockout. The exposed 16 A / 3680 W ceiling is the device rating, not
a claim that a destructive full-rating test was performed.

## Validation and provenance

The PA1/PC2/PB1 mapping, BL0937 polarity, measurement path, calibration, and
no-load behavior were validated on BSEED hardware before this upstream port.
Host tests cover derived measurements, calibration/converter scaling, overload
state transitions, and OTA payload identity; the Telink target is also built
with the exact BSEED configuration and defaults.

The metering implementation was adapted from
[`HobboRobin/tuya-zigbee-switch-with-metering` at `8b8cc492`](https://github.com/HobboRobin/tuya-zigbee-switch-with-metering/commit/8b8cc4924a353b35880666f7b48f0afbee89eb17).
The Telink pulse-counter HAL builds on the earlier upstream work in
[PR #314](https://github.com/romasku/tuya-zigbee-switch/pull/314).
