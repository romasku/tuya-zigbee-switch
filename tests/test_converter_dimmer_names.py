"""Dimmer channels in the generated Z2M converter: l1, l2 (as the stock definition of
zigbee-herdsman-converters names the ZDMS16-2's channels, state_l1 / switch_type_l1, so a module
moved from the stock firmware keeps the light's and the switch type's entities), and the
module's own settings on the device instead of on the first channel. Other boards keep their
settings on their first endpoint."""

import re

from generated_converter import custom_converter_js


def definitions() -> list[str]:
    js = custom_converter_js()
    body = js[js.index("const definitions = [") : js.index("module.exports")]
    return re.split(r"\n    \{\n(?=        zigbeeModel:)", body)[1:]


def model_of(definition: str) -> str:
    return re.search(r'^        model: "([^"]+)"', definition, re.M).group(1)


SETTINGS = ("deviceConfig", "deviceConfigExt", "mcuVersion", "multiPressResetCount")


def test_zdms16_2_channels_are_l1_and_l2():
    blocks = [d for d in definitions() if model_of(d) == "ZDMS16-2"]
    assert len(blocks) == 2
    for block in blocks:
        assert re.search(r'deviceEndpoints\(\{ endpoints: \{"l1": 1, "l2": 2, \} \}\)', block)
        assert re.findall(r'light\(\{ endpointNames: \["(\w+)"\]', block) == ["l1", "l2"]
        assert "dimmer_left" not in block and "dimmer_right" not in block


def test_a_dimmer_boards_own_settings_are_on_the_device():
    for block in (d for d in definitions() if model_of(d) == "ZDMS16-2"):
        for fn in SETTINGS:
            calls = re.findall(rf'romasku\.{fn}\(([^)]*)\)', block)
            assert len(calls) == 1 and "," not in calls[0], (fn, calls)   # a name, no endpoint


def test_other_boards_keep_their_settings_on_the_first_endpoint():
    others = [d for d in definitions() if model_of(d) != "ZDMS16-2"]
    assert len(others) == 212
    for block in others:
        first = re.search(r'deviceEndpoints\(\{ endpoints: \{"(\w+)": 1,', block).group(1)
        assert f'romasku.deviceConfig("device_config", "{first}")' in block, model_of(block)
        assert f'romasku.multiPressResetCount("multi_press_reset_count", "{first}")' in block, model_of(block)
