import re
import argparse

# Mostly AI-generated. Not extensively tested.

ALL_PINS = [f"{bank}{idx}" for bank in "ABCD" for idx in range(8)]
PIN_PATTERN = re.compile(r"NC|[ABCD][0-7]")


def choose(prompt: str, options: dict) -> str:
    while True:
        print(prompt)
        for key, label in options.items():
            print(f"  {key}) {label}")
        choice = input("Choose an option: ").strip()
        if choice in options:
            return choice
        print("Invalid choice, try again.\n")


def parse_cli_args():
    parser = argparse.ArgumentParser(
        description="Transpose Tuya pinouts (Interactive or arguments). "
        + "Useful if the same PCB is used with different modules (eg. ZT3L and ZS3L)"
    )
    parser.add_argument("direction", nargs="?", choices=["1", "2"],
                        help="1=Telink->Silabs, 2=Silabs->Telink")
    parser.add_argument("profile", nargs="?", choices=["1", "2", "3"],
                        help="1=ZTU/ZSU, 2=ZT3L/ZS3L, 3=ZT2S/ZS2S")
    parser.add_argument("text", nargs="?",
                        help='Input string to convert, e.g. "BA4u;LA3i;SB1u;RB0;"')

    return parser.parse_args()


def build_mapping_for_direction(base_map: dict[str, str], direction: str) -> dict[str, str | None]:
    if direction == "1":
        directed = dict(base_map)
    else:
        directed = {v: k for k, v in base_map.items()}

    # Add all possible source pins as unmapped (None), then overlay known mappings.
    full_map: dict[str, str | None] = {pin: None for pin in ALL_PINS}
    if "NC" in directed:
        full_map["NC"] = None
    full_map.update(directed)
    return full_map


def transpose(text: str, mapping: dict[str, str | None]) -> str:
    hits = PIN_PATTERN.findall(text)
    missing = sorted({pin for pin in hits if mapping.get(pin) is None})
    if missing:
        raise ValueError(
            "Unknown pin(s) for selected conversion: " + ", ".join(missing)
        )

    return PIN_PATTERN.sub(lambda m: mapping[m.group(0)] or m.group(0), text)


def main():
    first_level = {
        "1": "Telink to Silabs",
        "2": "Silabs to Telink",
    }

    second_level_telink = {
        "1": "ZTU  -> ZSU",
        "2": "ZT2S -> ZS2S",
        "3": "ZT3L -> ZS3L",
    }

    second_level_silabs = {
        "1": "ZSU  -> ZTU",
        "2": "ZS2S -> ZT2S",
        "3": "ZS3L -> ZT3L",
    }

    base_maps = {
        "1": {
            "NC": "C3",
            "D4": "C2",
            "C1": "D1",
            "C4": "D2",
            "B7": "A6",
            "B1": "A5",
            "B5": "A4",
            "B4": "A3",
            "D2": "A0",
            "C3": "B0",
            "C2": "B1",
            "A1": "D4",
            "A0": "D3",
            "B6": "C1",
            "A7": "A1",
            "C0": "A2",
            "D7": "C5",
            "D3": "C4",
        },
        "2": {
            "C4": "C1",
            "D2": "A4",
            "C3": "A3",
            "C2": "A0",
            "B7": "A6",
            "B1": "A5",
            "B4": "B0",
            "B5": "B1",
        },
        "3": {
            "C4": "C1",
            "D7": "C0",
            "D2": "A0",
            "C3": "A3",
            "C2": "A4",
            "C0": "D1",
            "D4": "D0",
            "A0": "C2",
            "B4": "B0",
            "B5": "B1",
            "B7": "A6",
            "B1": "A5",
        },
    }

    args = parse_cli_args()

    choice1 = args.direction if args.direction else choose("Transpose Tuya pinout:", first_level)

    if choice1 == "1":
        prompt = "Transpose Tuya pinout - Telink to Silabs:"
        level2 = second_level_telink
    else:
        prompt = "Transpose Tuya pinout - Silabs to Telink:"
        level2 = second_level_silabs

    choice2 = args.profile if args.profile else choose(prompt, level2)
    conversion = level2[choice2]

    user_string = args.text if args.text is not None else input(
        f"\nEnter a string to convert ({conversion}):\n"
    )

    mapping = build_mapping_for_direction(base_maps[choice2], choice1)

    try:
        result = transpose(user_string, mapping)
    except ValueError as e:
        print(f"\nError: {e}")
        return

    print(f"\n{result} - transposed")


if __name__ == "__main__":
    main()