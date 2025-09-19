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
        description="Create ZHA quirk for custom devices",
        epilog="Generates a py file that adds support of re-flashed devices to ZHA",
    )
    parser.add_argument(
        "db_file", metavar="INPUT", type=str, help="File with device db"
    )

    args = parser.parse_args()

    db_str = Path(args.db_file).read_text()
    db = yaml.safe_load(db_str)

    configs = []

    for device in db.values():

        # Skip if build == no. Defaults to yes
        if not device.get("build", True):
            continue

        configs.append(device["config_str"]);
        
    template = env.get_template("zha_quirk.py.jinja")

    print(template.render(configs=configs))

    exit(0)

    

