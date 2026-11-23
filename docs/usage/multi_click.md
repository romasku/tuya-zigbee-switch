# Momentary switch gestures (phase 1)

Gestures apply to momentary and normally closed momentary switches. A toggle
switch still reports its physical position. Gesture settings are per switch and
survive a restart. The existing relay/switch settings retain their NVM layout.

On the AVATTO LZWSM16-1 use the updated `switch_custom.js` converter in
Zigbee2MQTT. Ensure switch endpoint 1 (`switch`) has its `genMultistateInput`
binding to the coordinator and attribute reporting configured (the converter's
Configure action sets these up). Use the `action` event, not the diagnostic
`switch_press_action_switch` state, to drive automations:

- `switch_single`, `switch_double`, `switch_triple`
- `switch_single_hold` / `switch_single_release`, and equivalent double/triple
  variants when the last press of a sequence is held

The converter ignores transient press edges for `action`. Older firmware does
not emit confirmed click events; its existing diagnostic values still decode.
The converter publishes no action on an attribute read response.

Set `max_press_count_switch` to **2** for double clicks or **3** to include
triple clicks. The firmware default is **1**, which reports a single click on
debounced release without waiting. With a limit above 1, the switch waits
`confirm_release_ms_switch` after each short release, even at the maximum count
(extra clicks in the same sequence saturate at the limit). The default window
is **200 ms**. It measures debounced events, not the physical edge times. A
hold uses the existing long-press threshold. The native configuration attributes
in `genOnOffSwitchCfg` are `0xff07` (uint8, 1–3) and `0xff08` (uint16, 1–2000 ms).
Writing zero restores the default for that setting.

**Binding behavior is unchanged.** In `short_press` binding mode an On/Off
command still goes out on **each** short release, even if the gesture is later
reported as a double or triple. Relay modes and Move/StopWithOnOff behavior
remain independent of gesture reporting. This build has no Scenes client.

## Verification without hardware

- Run `make tests` for the router/End Device stubs, including gesture timing,
  per-endpoint settings, legacy NVM and the AVATTO detached/SHORT regression.
- From the repository root, run `node tests/test_switch_gesture_converter.cjs`
  with `zigbee-herdsman-converters` 26.125.0 and `zigbee-herdsman` 11.2.0 on the
  Node module path (Zigbee2MQTT 2.14.3 dependencies).
- With the Telink SDK/toolchain and `yq` installed, run
  `make -f board.mk build-firmware BOARD=MODULE_AVATTO_TS0011 DEVICE_TYPE=end_device BIN_PATH=build/phase1-avatto`.
  This builds the binary without publishing an OTA or modifying the OTA index.

These checks do not validate radio delivery, missed GPIO interrupts, or rollback
on an installed module. A controlled OTA test requires a separate decision.
Zigbee2MQTT 1.42.0 is not covered: its converters library lacks the `text`
modern extend that the upstream legacy converter already required.
