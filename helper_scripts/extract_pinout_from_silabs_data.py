
import argparse
from pathlib import Path

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Extract pinout from Tuya Silabs stock user_data dump",
        epilog="Generates config string",
    )
    parser.add_argument(
        "user_data", metavar="INPUT", type=str, help="File with stock user_data"
    )

    args = parser.parse_args()

    user_data = Path(args.user_data).read_bytes()

    start_pos = user_data.find(b"{")
    end_pos = user_data.find(b"}", start_pos)
    if start_pos == -1 or end_pos == -1:
        print("No config found")
        exit(1)
    
    config = user_data[start_pos + 1:end_pos]

    entries = [
        entry.split(":") for entry in config.decode().split(",")
        if entry
    ]

    config_dict = {
        key: value for key, value in entries
    }

    custom_config = ""

    # Confirmed on ZSU, ZS3L (EFR32MG21)
    pin_map = {
        "1": "A0",
        "2": "A1",
        "3": "A2",
        "4": "A3",
        "5": "A4",
        "6": "A5",
        "7": "A6",
        "8": "B0",
        "9": "B1",
        "10": "C0",
        "11": "C1",
        "12": "C2",
        "13": "C3",
        "14": "C4",
        "15": "C5",
        "16": "D0",
        "17": "D1",
        "18": "D2",
        "19": "D3",
        "20": "D4",
        "21": "D5",
    }

    if "total_bt_pin" in config_dict:
        pin = pin_map[config_dict["total_bt_pin"]]
        custom_config += f"B{pin}u;"
    if "netled1_pin" in config_dict:
        pin = pin_map[config_dict["netled1_pin"]]
        custom_config += f"L{pin};"

    for gang_index in range(1, 5):
        btn_key = f"bt{gang_index}_pin"
        if btn_key in config_dict:
            pin = pin_map[config_dict[btn_key]]
            custom_config += f"S{pin}u;"
        btn_key = f"i_bt{gang_index}"
        if btn_key in config_dict:
            pin = pin_map[config_dict[btn_key]]
            custom_config += f"S{pin}u;"
        relay_key = f"rl{gang_index}_pin"
        if relay_key in config_dict:
            pin = pin_map[config_dict[relay_key]]
            custom_config += f"R{pin};"
        relay_on_key = f"rl_on{gang_index}_pin"
        relay_off_key = f"rl_off{gang_index}_pin"
        if relay_on_key in config_dict and relay_off_key in config_dict:
            on_pin = pin_map[config_dict[relay_on_key]]
            off_pin = pin_map[config_dict[relay_off_key]]
            custom_config += f"R{on_pin}{off_pin};"
        led_key = f"led{gang_index}_pin"
        if led_key in config_dict:
            pin = pin_map[config_dict[led_key]]
            custom_config += f"I{pin};"
    
    print(config_dict)
    print(custom_config)