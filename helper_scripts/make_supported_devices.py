import argparse
from jinja2 import Environment, FileSystemLoader, select_autoescape
from pathlib import Path
import yaml


env = Environment(
    loader=FileSystemLoader("helper_scripts/templates"),
    autoescape=select_autoescape(),
    trim_blocks=True,
    lstrip_blocks=True,
)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Update supported_devices.md to with all entries from device_db.yaml",
    )
    parser.add_argument(
        "db_file", metavar="INPUT", type=str, help="File with device db"
    )

    args = parser.parse_args()

    db_str = Path(args.db_file).read_text()
    db = yaml.safe_load(db_str)

    devices = list(db.values())

    by_manufacturer_names = {}

    for device in devices:
        manufacturer_name = device["tuya_manufacturer_name"]
        if manufacturer_name not in by_manufacturer_names:
            by_manufacturer_names[manufacturer_name] = {
                **device,
                "tuya_manufacturer_name": manufacturer_name,
                "device_types": [device["device_type"]],
                "z2m_device": device.get("override_z2m_device") or device["stock_converter_model"]
            }
        if (device["device_type"] == "end_device"):
             by_manufacturer_names[manufacturer_name]["device_types"].append("router")

    for device in by_manufacturer_names.values():
        device["device_types"] = sorted(list(set(device["device_types"])), reverse=True)

    template = env.get_template("supported_devices.md.jinja")

    print(template.render(devices=by_manufacturer_names.values()))

    exit(0)

    

