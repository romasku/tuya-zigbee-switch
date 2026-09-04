"""Every two-letter config token must reach the z2m validator.

This has now bitten four times: ST swallowed by S, IT by I, RT rejected
outright, and PT by the dimmer map P. The pattern is always the same -- a token
is added to the C parser, and the JavaScript validator either has no branch for
it or has one placed after the single-letter branch that shares its first
character, so the generated converter refuses a config the firmware accepts.

Reviewing for it clearly does not work, so this asserts it structurally.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PARSER = ROOT / "src" / "device_config" / "config_parser.c"
CONVERTER = ROOT / "zigbee2mqtt" / "converters" / "switch_custom.js"


def two_letter_tokens() -> set[str]:
    src = PARSER.read_text(encoding="utf-8")
    pairs = re.findall(
        r"entry\[0\] == '([A-Za-z])' && entry\[1\] == '([A-Za-z])'", src
    )
    return {a + b for a, b in pairs}


def test_validator_knows_every_two_letter_token():
    js = CONVERTER.read_text(encoding="utf-8")
    # Accept either idiom the validator uses: startsWith('RT') for prefixes,
    # or an exact comparison such as part == 'SLP'.
    missing = [
        tok for tok in sorted(two_letter_tokens())
        if f"'{tok}" not in js
    ]
    assert not missing, (
        f"tokens the firmware parses but the validator rejects: {missing}"
    )


def test_two_letter_branches_come_before_their_single_letter_branch():
    js = CONVERTER.read_text(encoding="utf-8")
    # Only look at the validator, not the whole file.
    start = js.index("const validatePin")
    end = js.index("entityCategory", start)
    validator = js[start:end]

    for tok in sorted(two_letter_tokens()):
        two = validator.find(f"startsWith('{tok}')")
        if two == -1:
            continue  # covered by the test above
        one = validator.find(f"part[0] == '{tok[0]}'")
        if one == -1:
            continue  # no single-letter branch to be shadowed by
        assert two < one, (
            f"{tok} is checked after the bare '{tok[0]}' branch, so "
            f"'{tok[0]}' swallows it and {tok} configs are rejected"
        )
