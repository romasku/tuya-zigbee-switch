"""Print a targeted check payload after verifying the AVATTO role in bridge/devices."""

import argparse
import json
from pathlib import Path
import re


def make_check_payload(devices, ieee_address):
    if not re.fullmatch(r"0x[0-9a-fA-F]{16}", ieee_address):
        raise ValueError("Expected a Zigbee IEEE address (0x followed by 16 hex digits)")
    if not isinstance(devices, list):
        raise ValueError("Expected the JSON array from Zigbee2MQTT bridge/devices")
    ieee_address = ieee_address.lower()
    matches = [d for d in devices if isinstance(d, dict)
               and str(d.get("ieee_address", "")).lower() == ieee_address]
    if len(matches) != 1:
        raise ValueError("Target must identify exactly one device")
    device = matches[0]
    if device.get("type") != "EndDevice":
        raise ValueError("Target is not confirmed as EndDevice; do not use this image")
    if device.get("interview_completed") is not True:
        raise ValueError("Target must have completed its Zigbee interview")
    if device.get("model_id") not in {"TS0011-avatto", "TS0011-avatto-ED"} or device.get("manufacturer") not in {
        "hbxsdd6k", "TS0011-Avatto",
    }:
        raise ValueError("Target is not the romasku AVATTO LZWSM16-1")
    if device.get("software_build_id") != "1.1.3-ec57f81b":
        raise ValueError("Target does not report the expected stable build 1.1.3-ec57f81b")
    return {"id": ieee_address, "url": "avatto-phase1/index.json"}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bridge_devices_json", type=Path)
    parser.add_argument("ieee_address")
    args = parser.parse_args()
    try:
        payload = make_check_payload(json.loads(args.bridge_devices_json.read_text()), args.ieee_address)
    except (OSError, ValueError) as error:
        parser.exit(1, f"OTA preflight refused: {error}\n")
    print(json.dumps(payload))
