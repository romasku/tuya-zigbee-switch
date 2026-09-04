"""Every config string in the database must survive its own z2m validator.

The token-parity test only checks that a branch exists. It passed while the
RT branch carried /^RT[0-9A-Fa-f]{2}$/ -- two hex digits -- and the firmware
had long accepted RT<state><countdown> with four. So the TPZ-2's own config
string, straight out of device_db.yaml, could never be written over the air.

This pulls the real regexes out of the generated converter and runs every
board's config through them.
"""
import re
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
DB = ROOT / "device_db.yaml"
CONVERTER = ROOT / "zigbee2mqtt" / "converters" / "switch_custom.js"


def branch_regexes() -> dict[str, re.Pattern]:
    """prefix -> regex, read from the generated validator."""
    js = CONVERTER.read_text(encoding="utf-8")
    start = js.index("const validatePin")
    end = js.index("entityCategory", start)
    validator = js[start:end]

    out = {}
    # Split into branches first: a prefix branch may validate with validatePin
    # instead of a regex, and a naive search would then pick up the next
    # branch's regex and check the token against the wrong rule.
    branches = re.split(r"\}\s*else if\s*", validator)
    for branch in branches:
        m = re.match(r"\(part\.startsWith\('([A-Z]{2})'\)\)", branch)
        if not m:
            continue
        rx = re.search(r"/\^([^/]+)/\.test\(part\)", branch)
        if rx:
            out[m.group(1)] = re.compile("^" + rx.group(1))
    return out


def config_strings() -> list[tuple[str, str]]:
    db = yaml.safe_load(DB.read_text(encoding="utf-8"))
    return [
        (board, entry["config_str"])
        for board, entry in db.items()
        if isinstance(entry, dict) and entry.get("config_str")
    ]


def test_every_config_string_passes_the_generated_validator():
    regexes = branch_regexes()
    assert regexes, "no prefixed branches found -- the extractor is broken"

    failures = []
    for board, cfg in config_strings():
        for part in cfg.rstrip(";").split(";")[2:]:
            prefix = part[:2]
            rx = regexes.get(prefix)
            if rx and not rx.match(part):
                failures.append(f"{board}: {part} rejected by /{rx.pattern}/")

    assert not failures, "config strings the validator would refuse:\n" + "\n".join(
        failures
    )
