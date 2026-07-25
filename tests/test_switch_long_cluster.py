from dataclasses import dataclass

import pytest

from tests.conftest import Device, RelayButtonPair, StubProc
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_DIRECTION,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MOVE_COMMAND,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    ZCL_LEVEL_MOVE_DIRECTION_ALTERNATE,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE,
)


@dataclass
class AttrValueTestCase:
    name: str
    cluster: int
    attr: int
    expected: str


ATTR_TEST_CASES = [
    AttrValueTestCase(
        name="Switch Type",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE,
        expected=str(ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE),
    ),
    AttrValueTestCase(
        name="Switch Actions",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        expected=str(ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE),
    ),
    AttrValueTestCase(
        name="Binded Mode",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
        expected=str(ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG),
    ),
    AttrValueTestCase(
        name="Relay Mode",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
        expected=str(ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED),
    ),
    AttrValueTestCase(
        name="Move Command",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MOVE_COMMAND,
        expected=str(ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF),
    ),
    AttrValueTestCase(
        name="Level Move Direction",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_DIRECTION,
        expected=str(ZCL_LEVEL_MOVE_DIRECTION_ALTERNATE),
    ),
    AttrValueTestCase(
        name="Level Move Rate",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
        expected="50",
    ),
]


@pytest.mark.parametrize("case", ATTR_TEST_CASES, ids=lambda case: case.name)
def test_switch_long_cluster_read_attrs(
    device: Device,
    relay_button_pairs: list[RelayButtonPair],
    case: AttrValueTestCase,
):
    for pair in relay_button_pairs:
        assert (
            device.read_zigbee_attr(pair.long_press_endpoint, case.cluster, case.attr)
            == case.expected
        )


def test_switch_long_cluster_read_relay_index(
    device: Device, relay_button_pairs: list[RelayButtonPair]
):
    for pair in relay_button_pairs:
        expected = str(pair.switch_endpoint)
        assert (
            device.read_zigbee_attr(
                pair.long_press_endpoint,
                ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
                ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
            )
            == expected
        )


@pytest.mark.parametrize(
    "switch_type",
    [
        ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE,
        ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
    ],
    ids=["toggle", "momentary"],
)
def test_switch_long_cluster_switch_type_mirrors_short_ep(
    device: Device, relay_button_pair: RelayButtonPair, switch_type: int
):
    """SwitchType on long_press_ep mirrors the value on switch_ep —
    it's a button-wide physical characteristic, not per-endpoint."""
    device.zcl_switch_mode_set(relay_button_pair.switch_endpoint, switch_type)

    actual = int(device.read_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE,
    ))
    assert actual == switch_type


def test_switch_long_cluster_binded_mode_is_read_only(
    device: Device, relay_button_pair: RelayButtonPair
):
    """binded_mode on long_press_ep is a constant LONG marker — writes either
    silently ignored or rejected; either way the value never changes."""
    # Bypass write_zigbee_attr (which asserts ok) — FW may legitimately reject.
    device.p.exec(
        f"zcl_write {relay_button_pair.long_press_endpoint} "
        f"0x{ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG:04X} "
        f"0x{ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE:04X} "
        f"{ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE}"
    )

    actual = int(device.read_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
    ))
    assert actual == ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG


@pytest.mark.parametrize(
    "device_config,expected_long_press_eps",
    [
        ("A;B;SA0u;RB0;", [3]),
        ("A;B;SA0u;SA1u;RB0;RB1;", [5, 6]),
        ("A;B;SA0u;SA1u;SA2u;RB0;RB1;RB2;", [7, 8, 9]),
    ],
    ids=["1-gang", "2-gang", "3-gang"],
)
def test_switch_long_cluster_endpoint_layout(
    device_config: str, expected_long_press_eps: list[int]
) -> None:
    """For N switches + N relays + 0 covers, long_press_eps live at
    ep 2N+1 .. 3N — verified via the binded_mode=LONG marker on each."""
    with StubProc(device_config=device_config) as proc:
        device = Device(proc)

        for ep in expected_long_press_eps:
            actual = int(device.read_zigbee_attr(
                ep,
                ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
                ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
            ))
            assert actual == ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG, (
                f"ep{ep} is not a long_press_ep (binded_mode={actual:#x}, "
                f"expected {ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG:#x})"
            )


def test_switch_long_cluster_attributes_preserved_via_nvm() -> None:
    """Test that writable long_press_ep attributes are preserved across device restarts via NVM"""
    device_config = "A;B;SA0u;SA1u;RB0;RB1;"  # Two switches and two relays
    long_press_endpoint = 5  # switches=2, relays=2 => long_press_ep[0] = 5

    test_values = {
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS: "1",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE: "2",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX: "2",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MOVE_COMMAND: "1",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_DIRECTION: "0",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE: "100",
    }

    # First session: write all test values
    with StubProc(device_config=device_config) as proc:
        device = Device(proc)

        for attr_id, value in test_values.items():
            device.write_zigbee_attr(
                long_press_endpoint, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, attr_id, value
            )
    # Second session: restart device and verify values are preserved
    with StubProc(device_config=device_config) as proc:
        device = Device(proc)

        for attr_id, expected_value in test_values.items():
            actual_value = device.read_zigbee_attr(
                long_press_endpoint, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, attr_id
            )
            assert actual_value == expected_value, (
                f"Attribute {attr_id:04x} not preserved via NVM: expected {expected_value}, got {actual_value}"
            )


@pytest.mark.parametrize(
    "invalid_relay_mode",
    [
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT,
    ],
)
def test_switch_long_cluster_relay_mode_invalid_values(
    device: Device,
    relay_button_pair: RelayButtonPair,
    invalid_relay_mode: int,
):
    device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint, invalid_relay_mode
    )

    device.long_click_button(relay_button_pair.button_pin)

    device.step_time(100)

    # RISE/SHORT are not valid on long_press_ep — writes clamp to DETACHED.
    clamped = int(device.read_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
    ))
    assert clamped == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED


@pytest.mark.parametrize(
    "invalid_relay_index",
    [0, 100, 255],
)
def test_switch_long_cluster_relay_index_invalid_values(
    device: Device,
    relay_button_pair: RelayButtonPair,
    invalid_relay_index: int,
):
    device.write_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
        invalid_relay_index,
    )

    device.long_click_button(relay_button_pair.button_pin)

    device.step_time(100)

    # Out-of-range writes clamp to a valid relay_index (>= 1, <= relay_clusters_cnt).
    clamped = int(device.read_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
    ))
    assert clamped != invalid_relay_index
    assert 1 <= clamped


@pytest.mark.parametrize("invalid_move_command", [0x00, 0x02, 0x42, 0xFF])
def test_switch_long_cluster_move_command_invalid_values(
    device: Device,
    relay_button_pair: RelayButtonPair,
    invalid_move_command: int,
):
    device.write_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MOVE_COMMAND,
        invalid_move_command,
    )

    device.long_click_button(relay_button_pair.button_pin)

    device.step_time(100)

    # Only Move (0x01) and MoveWithOnOff (0x05) are valid — others clamp to MoveWithOnOff.
    clamped = int(device.read_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MOVE_COMMAND,
    ))
    assert clamped == ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF


@pytest.mark.parametrize("invalid_direction", [0x02, 0x42, 0x7F])
def test_switch_long_cluster_level_move_direction_invalid_values(
    device: Device,
    relay_button_pair: RelayButtonPair,
    invalid_direction: int,
):
    device.write_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_DIRECTION,
        invalid_direction,
    )

    device.long_click_button(relay_button_pair.button_pin)

    device.step_time(100)

    # Only Up (0x00), Down (0x01), Alternate (0xFF) are valid — others clamp to Alternate.
    clamped = int(device.read_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_DIRECTION,
    ))
    assert clamped == ZCL_LEVEL_MOVE_DIRECTION_ALTERNATE
