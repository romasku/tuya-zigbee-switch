# Zigbee Endpoints in Firmware

> Read this document if you want to directly bind the device or add it to a group.

As the firmware supports multi-channel (multi-gang) devices, it uses Zigbee endpoints to handle command routing. Zigbee endpoints are numbered, starting from one. For each endpoint, only one instance of a specific function can exist. For example, there can only be a single relay (`OnOffCluster`) attached to endpoint 1. This document explains how the firmware assigns and uses endpoints.

If the device is an N-gang switch module, the firmware will use `2 × N` endpoints. The first N endpoints are used for "client" (output) OnOff clusters, which can control other Zigbee devices via direct bindings. The next N endpoints (endpoints N+1 to 2×N) are used for "server" (input) OnOff clusters, which are directly linked to physical relays.

Here is an example table:

| Endpoint | Clusters     | Description                                                                                  |
|----------|--------------|----------------------------------------------------------------------------------------------|
| 1        | OnOff client | Binding to control other Zigbee devices                                                      |
| ...      | OnOff client | ...                                                                                          |
| N        | OnOff client | Binding to control other Zigbee devices                                                      |
| N+1      | OnOff server | Controls Relay 1 state. Add to a group or bind it with another device to control the relay.  |
| ...      | OnOff server | ...                                                                                          |
| 2N       | OnOff server | Controls Relay N state. Add to a group or bind it with another device to control the relay.  |

## Usage Examples

### Controlling a Smart Bulb

If you have a 2-gang module and want its second button to control a smart bulb via Zigbee direct communication:

Bind endpoint 2 of your device to endpoint 1 of the bulb, and bind the `OnOff` cluster, as shown in the screenshot:

![bulb binding](<images/bind_bulb.png>)

### Creating a Zigbee Group

If you have two 2-gang devices and want to group the first relay of both devices, you should add endpoint 3 of both devices to the same group, as shown in the screenshot:

![add to group](<images/add_to_group.png>)
