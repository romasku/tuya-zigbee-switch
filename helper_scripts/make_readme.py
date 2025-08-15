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
        description="Update readme file to include full list of supported deviced",
    )
    parser.add_argument(
        "db_file", metavar="INPUT", type=str, help="File with device db"
    )
    parser.add_argument(
        "version", metavar="VERSION", type=str, help="Version number"
    )

    args = parser.parse_args()

    db_str = Path(args.db_file).read_text()
    db = yaml.safe_load(db_str)

    devices = list(db.values())

    by_manufacturer_names = {}

    for device in devices:
        manufacturer_name = next(name for name in device["tuya_manufacturer_names"] if name.startswith("_TZ3000"))
        if manufacturer_name not in by_manufacturer_names:
            by_manufacturer_names[manufacturer_name] = {
                **device,
                "tuya_manufacturer_name": manufacturer_name,
                "device_types": [device["device_type"]],
                "z2m_device": device.get("override_z2m_device") or device["stock_converter_model"]
            }
        else:
             by_manufacturer_names[manufacturer_name]["device_types"].append(device["device_type"])

    for device in by_manufacturer_names.values():
        device["device_types"] = sorted(list(set(device["device_types"])), reverse=True)

    template = env.get_template("readme.md.jinja")

    print(template.render(devices=by_manufacturer_names.values(), version=args.version))

    exit(0)

    

