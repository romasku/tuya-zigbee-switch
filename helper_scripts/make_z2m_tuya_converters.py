import argparse
from jinja2 import Environment, FileSystemLoader, select_autoescape
from pathlib import Path
import yaml

env = Environment(
    loader=FileSystemLoader("helper_scripts/templates"),
    autoescape=select_autoescape(),
    trim_blocks=True,
    lstrip_blocks=True
)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create Zigbee2mqtt converter for tuya devices with ota",
        epilog="Generates a js file that adds ota support for given tuya models")
    parser.add_argument(
        "db_file", metavar="INPUT", type=str, help="File with device db"
    )
    parser.add_argument(
        "--z2m-v1", action=argparse.BooleanOptionalAction, help="Use old z2m"
    )


    args = parser.parse_args()

    db_str = Path(args.db_file).read_text()
    db = yaml.safe_load(db_str)

    manufacturers = {
        "Tuya": ["TS0001", "TS0002", "TS0003", "TS0004"],
        "Moes": [],
        "AVATTO": []
    }

    # Prepare containers for multiple pinout models
    multiple_pinouts = {key: [] for key in manufacturers}

    for entry in db.values():
      
        # Skip if build == no. Defaults to yes
        if not entry.get("build", True):
            continue
      
        model = entry.get("stock_converter_model")
        mfr = entry.get("stock_converter_manufacturer", "Tuya")
        if model is None or mfr not in manufacturers:
            continue

        manufacturers[mfr].append(model)

        if entry.get("alt_config_str"):
            multiple_pinouts[mfr].append(model)

    tuyaModels = manufacturers["Tuya"]
    moesModels = manufacturers["Moes"]
    avattoModels = manufacturers["AVATTO"]

    tuyaMultiplePinoutsModels = multiple_pinouts["Tuya"]
    moesMultiplePinoutsModels = multiple_pinouts["Moes"]
    avattoMultiplePinoutsModels = multiple_pinouts["AVATTO"]

    template = env.get_template("tuya_with_ota.js.jinja")

    print(template.render(
        tuyaModels=sorted(list(set(tuyaModels))),
        tuyaMultiplePinoutsModels=sorted(list(set(tuyaMultiplePinoutsModels))),
        moesModels=sorted(list(set(moesModels))),
        moesMultiplePinoutsModels=sorted(list(set(moesMultiplePinoutsModels))),
        avattoModels=sorted(list(set(avattoModels))),
        avattoMultiplePinoutsModels=sorted(list(set(avattoMultiplePinoutsModels))),
         z2m_v1=args.z2m_v1)
    )
   
    exit(0)