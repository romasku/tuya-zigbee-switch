# Multi-click on momentary switches

A momentary switch can report single, double, and triple clicks for use in automations. You can also hold the last press in any of these sequences to get a hold event followed by a release event. This also works with normally closed momentary switches. Latching switches still report their physical position instead.

## Set up multi-click in Zigbee2MQTT

Use a converter that supports multi-click on your firmware. If action events are missing, reconfigure the device in Zigbee2MQTT so the button endpoint's `genMultistateInput` cluster is bound to the coordinator.

- **Max press count** (`max_press_count_switch`): set to **2** for double clicks or **3** for double and triple clicks. The default is **1**, which reports each short click on release without an extra wait.
- **Confirm release** (`confirm_release_ms_switch`): how long the switch waits after a short release for the next press when the click limit is above 1. The default is **200 ms**. Increase it if your double clicks are being reported as two singles.

These settings are saved per button. On a multi-button device, the field suffix identifies the button you are configuring.

## Use the action events

Trigger automations from the Zigbee2MQTT `action` event, not the diagnostic press state (`switch_press_action_switch`). For a button named `switch`, short clicks produce `switch_single`, `switch_double`, or `switch_triple`.

If the last press in a sequence is held, the switch reports a hold and then a release instead of a short-click event. For example, a short click followed by a held second press produces `switch_double_hold` and then `switch_double_release`. The hold threshold uses the existing long-press setting.

## How multi-click affects bindings

Multi-click reporting does not change local relay behavior or when the switch sends Zigbee commands to bound devices. With `short_press` binding mode and `toggle_simple` action, a double click reports one `switch_double` event but still sends two Toggle commands, one per short release. A bound light can therefore briefly change state and then return to its original state.
