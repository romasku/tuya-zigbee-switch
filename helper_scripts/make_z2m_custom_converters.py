import argparse
from pathlib import Path

import yaml
from jinja2 import Environment, FileSystemLoader, select_autoescape

env = Environment(
    loader=FileSystemLoader("helper_scripts/templates"),
    autoescape=select_autoescape(),
    trim_blocks=True,
    lstrip_blocks=True,
)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Create Zigbee2mqtt converter for custom devices",
        epilog="Generates an mjs file that adds support of re-flashed devices to z2m",
    )
    parser.add_argument(
        "db_file", metavar="INPUT", type=str, help="File with device db"
    )

    args = parser.parse_args()

    db_str = Path(args.db_file).read_text()
    db = yaml.safe_load(db_str)

    devices = []

    for device in db.values():
        # Skip if build == no. Defaults to yes
        if not device.get("build", True):
            continue

        _, zb_model, *_ = device["config_str"].rstrip(";").split(";")

        devices.append(
            {
                "zb_models": [zb_model] + (device.get("old_zb_models") or []),
                "model": device.get("override_z2m_device")
                or device["stock_converter_model"],
            }
        )

    template = env.get_template("switch_custom.mjs.jinja")

    print(template.render(devices=devices))

    exit(0)
