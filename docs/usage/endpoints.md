# Zigbee Endpoints in Firmware

> Read this document if you want to directly bind the device or add it to a group.

As the firmware supports multi-channel (multi-gang) devices, it uses Zigbee endpoints to handle command routing. Zigbee endpoints are numbered, starting from one. For each endpoint, only one instance of a specific function can exist. For example, there can only be a single relay (`OnOffCluster`) attached to endpoint 1. This document explains how the firmware assigns and uses endpoints.

If the device is an N-gang switch module, the firmware uses up to `3 × N` endpoints. The first N endpoints are "client" (output) OnOff clusters for direct bindings. If the device has physical relays, endpoints `N+1`..`2N` are "server" (input) OnOff clusters linked to those relays — on relay-less devices these endpoints are absent. Endpoints `2N+1`..`3N` are long-press companion "client" (output) OnOff + LevelControl clusters that fire on long-press.

Here is an example table:

| Endpoint | Clusters                       | Description |
|----------|--------------------------------|-------------|
| 1        | OnOff client                   | Binding to control other Zigbee devices on short press |
| ...      | OnOff client                   | ... |
| N        | OnOff client                   | Binding to control other Zigbee devices |
| N+1      | OnOff server                   | Controls Relay 1 state. Add to a group or bind it with another device to control the relay. |
| ...      | OnOff server                   | ... |
| 2N       | OnOff server                   | Controls Relay N state. Add to a group or bind it with another device to control the relay. |
| 2N+1     | OnOff client + Level client    | Long-press companion for switch 1 |
| ...      | OnOff client + Level client    | ... |
| 3N       | OnOff client + Level client    | Long-press companion for switch N |

## Usage Examples

### Controlling a Smart Bulb

If you have a 2-gang module and want its second button to control a smart bulb via Zigbee direct communication:

Bind endpoint 2 of your device to endpoint 1 of the bulb, and bind the `OnOff` cluster, as shown in the screenshot:

![bulb binding](/docs/.images/bind_bulb.png)

### Creating a Zigbee Group

If you have two 2-gang devices and want to group the first relay of both devices, you should add endpoint 3 of both devices to the same group, as shown in the screenshot:

![add to group](/docs/.images/add_to_group.png)

### Long-press as a second binding target

Bind the `OnOff` cluster of endpoint `2N+1` (long-press companion of button 1) to a Zigbee target. Each long-press sends `Toggle` to that target — independently of any Home Assistant / Zigbee2MQTT automation.

## Migration from legacy long-press configuration

Earlier firmware reused the short-press endpoint for long-press via `binded_mode=LongPress` / `relay_mode=LongPress`. These are deprecated in favor of the dedicated long-press companion endpoints (`2N+1..3N`).

Existing configurations keep working: while a switch endpoint has either of those deprecated modes, its paired long-press companion is muted to prevent double-toggles.

**To migrate a button**:

1. Move any OnOff and LevelControl bindings from the switch endpoint (`1..N`) onto its long-press companion (`2N+1..3N`).
2. If `relay_mode=LongPress` was used on the switch endpoint, set `relay_mode=LongPress` on the companion (and configure `relay_index` if needed).
3. Set the switch endpoint's `binded_mode` and `relay_mode` to anything except `LongPress` — that releases the mute on the companion.
