# Multi-click on momentary switches

A momentary switch can report single, double, and triple clicks for use in automations. You can also hold the last press in any of these sequences to get a hold event followed by a release event. This also works with normally closed momentary switches. Latching switches still report their physical position instead.

## Set up multi-click in Zigbee2MQTT

Use a converter that supports multi-click on your firmware. If action events are missing, reconfigure the device in Zigbee2MQTT so the button endpoint's `genMultistateInput` cluster is bound to the coordinator.

- **Max press count** (`max_press_count_switch`): set to **2** for double clicks or **3** for double and triple clicks. The default is **1**, which reports each short click on release without an extra wait.
- **Confirm release** (`confirm_release_ms_switch`): how long the switch waits after a short release for the next press when the click limit is above 1. The default is **200 ms**. Increase it if your double clicks are being reported as two singles.

These settings are saved per button. On a multi-button device, the field suffix identifies the button you are configuring.

## Use multi-click events in automations

Use multi-click events as automation triggers rather than the switch's current press state. How these events are exposed depends on your Zigbee integration. The table shows the Zigbee2MQTT `action` values for a button named `switch`; other buttons use their own endpoint name as the prefix.

| Presses | All presses short | Last press held | When the held press is released |
| --- | --- | --- | --- |
| 1 | `switch_single` | `switch_single_hold` | `switch_single_release` |
| 2 | `switch_double` | `switch_double_hold` | `switch_double_release` |
| 3 | `switch_triple` | `switch_triple_hold` | `switch_triple_release` |

If you hold the last press, you get the hold and release events instead of the short-click event. The hold threshold uses the existing long-press setting. A latching switch reports position changes instead of multi-click events (`switch_position_on` and `switch_position_off` in Zigbee2MQTT).

In Zigbee2MQTT, the diagnostic field `switch_press_action_switch` shows the current switch state: `released`, `press`, `long_press`, `position_on`, or `position_off`. It also shows `double_hold` or `triple_hold` while the second or third press is held past the long-press threshold.

## How multi-click affects bindings

Multi-click reporting does not change local relay behavior or when the switch sends Zigbee commands to bound devices. With `short_press` binding mode and `toggle_simple` action, a double click reports one `switch_double` event but still sends two Toggle commands, one per short release. A bound light can therefore briefly change state and then return to its original state.
