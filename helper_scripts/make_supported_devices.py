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
        description="Update supported.md to with all entries from device_db.yaml",
    )
    parser.add_argument(
        "db_file", metavar="INPUT", type=str, help="File with device db"
    )

    args = parser.parse_args()

    db_str = Path(args.db_file).read_text()
    db = yaml.safe_load(db_str)

    devices = db.items()

    template = env.get_template("supported_devices.md.jinja")
    print(template.render(devices=devices))
    
    exit(0)

    

