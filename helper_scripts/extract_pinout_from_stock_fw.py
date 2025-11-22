
import argparse
from pathlib import Path

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Extract pinout from stock firmware dump",
        epilog="Generates a js file that adds support of re-flashed devices to z2m",
    )
    parser.add_argument(
        "stock_fw", metavar="INPUT", type=str, help="File with stock firmware"
    )

    args = parser.parse_args()

    firmare_bin = Path(args.stock_fw).read_bytes()

    approx_config_part = firmare_bin[0xf8000:0xf9000]
    start_pos = approx_config_part.find(b"{")
    end_pos = approx_config_part.find(b"}", start_pos)
    if start_pos == -1 or end_pos == -1:
        print("No config found")
        exit(1)
    
    config = approx_config_part[start_pos + 1:end_pos]

    entries = [
        entry.split(":") for entry in config.decode().split(",")
        if entry
    ]

    config_dict = {
        key: value for key, value in entries
    }

    custom_config = ""

    pin_map = {
        "0": "A0",
        "4": "B4",
        "5": "B5",
        "6": "B6",
        "7": "B7",
        "8": "C0",
        "9": "C1",
        "10": "C2",
        "11": "C3",
        "12": "C4",
        "13": "D2",
        "14": "D3",
        "15": "D4",
        "16": "D7",
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