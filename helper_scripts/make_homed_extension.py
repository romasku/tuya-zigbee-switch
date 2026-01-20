import argparse
import json
import yaml

from pathlib import Path


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Create HOMEd extension for custom devices",
        epilog="Generates a json file that adds support of re-flashed devices to HOMEd",
    )
    parser.add_argument(
        "db_file", metavar="INPUT", type=str, help="File with device db"
    )

    args = parser.parse_args()

    db_str = Path(args.db_file).read_text()
    db = yaml.safe_load(db_str)

    data = {}

    for device in db.values():
        # Skip if build == no. Defaults to yes
        if not device.get("build", True):
            continue

        config = device["config_str"]
        zb_manufacturer, zb_model, *peripherals = config.rstrip(";").split(";")

        relay_cnt = 0
        switch_cnt = 0
        cover_cnt = 0
        indicators_cnt = 0
        has_dedicated_net_led = False
        for peripheral in peripherals:
            if peripheral == "SLP":
                continue
            if peripheral[0] == "R":
                relay_cnt += 1
            if peripheral[0] == 'S':
                switch_cnt += 1
            if peripheral[0] == 'C':
                cover_cnt += 1
            if peripheral[0] == 'I':
                indicators_cnt += 1
            if peripheral[0] == 'L':
                has_dedicated_net_led = True

        if switch_cnt == 1:
            switch_names = ["switch"]
        elif switch_cnt == 2:
            switch_names = ["switch_left", "switch_right"]
        elif switch_cnt == 3:
            switch_names = ["switch_left", "switch_middle", "switch_right"]
        else:
            switch_names = [f"switch_{index + 1}" for index in range(relay_cnt)]

        if relay_cnt == 1:
            relay_names = ["relay"]
        elif relay_cnt == 2:
            relay_names = ["relay_left", "relay_right"]
        elif relay_cnt == 3:
            relay_names = ["relay_left", "relay_middle", "relay_right"]
        else:
            relay_names = [f"relay_{index + 1}" for index in range(relay_cnt)]

        if cover_cnt == 1:
            cover_names = ["cover"]
        elif cover_cnt == 2:
            cover_names = ["cover_left", "cover_right"]
        elif cover_cnt == 3:
            cover_names = ["cover_left", "cover_middle", "cover_right"]
        else:
            cover_names = [f"cover_{index + 1}" for index in range(cover_cnt)]

        endpoint_names = switch_names + relay_names + cover_names
        if not endpoint_names:
            continue
        model_names = [zb_model] + (device.get("old_zb_models") or [])

        data.setdefault(zb_manufacturer, [])

        data[zb_manufacturer].append({
            "description": "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
            "modelNames": model_names,
            "options": {
                "endpointName": {
                    str(i + 1): name for i, name in enumerate(endpoint_names)
                }
            }
        })

        if has_dedicated_net_led:
            data[zb_manufacturer].append({
                "modelNames": model_names,
                "exposes": ["networkIndicator"],
                "options": {
                    "customAttributes": {
                        "networkIndicator": {"type": "bool", "clusterId": 0x0000, "attributeId": 0xff01, "dataType": 0x10, "action": True}
                    },
                    "networkIndicator": {"type": "toggle"}
                },
                "endpointId": 1
            })

        if relay_cnt:
            relay_endpoints = list(range(switch_cnt + 1, switch_cnt + 1 + relay_cnt))
            data[zb_manufacturer].append({
                "modelNames": model_names,
                "properties": ["status", "powerOnStatus"],
                "actions": ["status", "powerOnStatus"],
                "bindings": ["status"],
                "reportings": ["status"],
                "exposes": ["switch", "powerOnStatus"],
                "options": {
                    "powerOnStatus": {"enum": ["off", "on", "toggle", "previous"]}
                },
                "endpointId": relay_endpoints
            })

        if switch_cnt:
            switch_endpoints = list(range(1, 1 + switch_cnt))
            data[zb_manufacturer].append({
                "modelNames": model_names,
                "exposes": ["pressAction", "switchMode", "switchAction", "relayMode", "relayIndex", "bindedMode", "longPressDuration", "levelMoveRate"],
                "options": {
                    "customAttributes": {
                        "switchAction": {"type": "enum", "clusterId": 0x0007, "attributeId": 0x0010, "dataType": 0x30, "action": True},
                        "switchMode": {"type": "enum", "clusterId": 0x0007, "attributeId": 0xff00, "dataType": 0x30, "action": True},
                        "relayMode": {"type": "enum", "clusterId": 0x0007, "attributeId": 0xff01, "dataType": 0x30, "action": True},
                        "relayIndex": {"type": "enum", "clusterId": 0x0007, "attributeId": 0xff02, "dataType": 0x20, "action": True},
                        "longPressDuration": {"type": "value", "clusterId": 0x0007, "attributeId": 0xff03, "dataType": 0x21, "action": True},
                        "levelMoveRate": {"type": "value", "clusterId": 0x0007, "attributeId": 0xff04, "dataType": 0x20, "action": True},
                        "bindedMode": {"type": "enum", "clusterId": 0x0007, "attributeId": 0xff05, "dataType": 0x30, "action": True},
                        "pressAction": {"type": "enum", "clusterId": 0x0012, "attributeId": 0x0055, "dataType": 0x21, "binding": True,
                                        "reporting": {"minInterval": 0, "maxInterval": 65535, "valueChange": 1}}
                    },
                    "switchAction": {"type": "select", "enum": ["on_off", "off_on", "toggle_simple", "toggle_smart_sync", "toggle_smart_opposite"]},
                    "switchMode": {"type": "select", "enum": ["toggle", "momentary", "momentary_nc"]},
                    "relayMode": {"type": "select", "enum": ["detached", "press_start", "short_press", "long_press"]},
                    "relayIndex": {"type": "select", "enum": {str(i + 1): f"relay_{i + 1}" for i in range(relay_cnt)}},
                    "longPressDuration": {"type": "number", "min": 0, "max": 5000},
                    "levelMoveRate": {"type": "number", "min": 1, "max": 255},
                    "bindedMode": {"type": "select", "enum": {"1": "press_start", "2": "long_press", "3": "short_press"}},
                    "pressAction": {"enum": ["released", "press", "long_press", "position_on", "position_off"]}
                },
                "endpointId": switch_endpoints
            })

        if indicators_cnt:
            relay_indicator_endpoints = list(range(switch_cnt + 1, switch_cnt + 1 + indicators_cnt))
            data[zb_manufacturer].append({
                "modelNames": model_names,
                "exposes": ["relayIndicatorMode", "relayIndicator"],
                "options": {
                    "customAttributes": {
                        "relayIndicatorMode": {"type": "enum", "clusterId": 0x0006, "attributeId": 0xff01, "dataType": 0x30, "action": True},
                        "relayIndicator": {"type": "bool", "clusterId": 0x0006, "attributeId": 0xff02, "dataType": 0x10, "action": True,
                                           "reporting": {"minInterval": 0, "maxInterval": 255, "valueChange": 1}},
                    },
                    "relayIndicatorMode": {"type": "select", "enum": ["same", "opposite", "manual"]},
                    "relayIndicator": {"type": "toggle"}
                },
                "endpointId": relay_indicator_endpoints
            })

        if cover_cnt:
            cover_endpoints = list(range(switch_cnt + relay_cnt + 1, switch_cnt + relay_cnt + 1 + cover_cnt))
            data[zb_manufacturer].append({
                "modelNames": model_names,
                "properties": ["coverPosition"],
                "actions": ["coverStatus", "coverPosition"],
                "bindings": ["cover"],
                "reportings": ["coverPosition"],
                "exposes": ["cover", "coverMoving", "coverMotorReversal"],
                "options": {
                    "customAttributes": {
                        "coverMoving": {"type": "enum", "clusterId": 0x0102, "attributeId": 0xff00, "dataType": 0x30, "binding": True,
                                        "reporting": {"minInterval": 0, "maxInterval": 255, "valueChange": 1}},
                        "coverMotorReversal": {"type": "value", "clusterId": 0x0102, "attributeId": 0xff01, "dataType": 0x10, "action": True}
                    },
                    "coverMoving": {"enum": ["stopped", "opening", "closing"]},
                    "coverMotorReversal": {"type": "toggle"}
                },
                "endpointId": switch_endpoints
            })

    print(json.dumps(data, indent=2))

    exit(0)
