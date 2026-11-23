from dataclasses import dataclass

import pytest

from tests.conftest import Device, StubProc
from tests.zcl_consts import (
    ZCL_ATTR_MULTISTATE_INPUT_NUMBER_OF_STATES,
    ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_TYPE,
    ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
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
        name="Switch Mode",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
        expected=str(ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE),
    ),
    AttrValueTestCase(
        name="Relay Mode",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
        expected=str(ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT),
    ),
    AttrValueTestCase(
        name="Long Press Duration",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
        expected="800",
    ),
    AttrValueTestCase(
        name="Level Move Rate",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
        expected="50",
    ),
    AttrValueTestCase(
        name="Binding Mode",
        cluster=ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        attr=ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
        expected=str(ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT),
    ),
    AttrValueTestCase(
        name="Multistate Number of States",
        cluster=ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
        attr=ZCL_ATTR_MULTISTATE_INPUT_NUMBER_OF_STATES,
        expected="3",
    ),
    AttrValueTestCase(
        name="Multistate Present Value",
        cluster=ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
        attr=ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
        expected="0",
    ),
]


@pytest.mark.parametrize("case", ATTR_TEST_CASES, ids=lambda case: case.name)
def test_switch_cluster_read_attrs(
    device: Device, button_pins: list[str], case: AttrValueTestCase
):
    for endpoint, _ in enumerate(button_pins, start=1):
        assert (
            device.read_zigbee_attr(endpoint, case.cluster, case.attr) == case.expected
        )


def test_switch_cluster_read_relay_index(device: Device, button_pins: list[str]):
    for endpoint, _ in enumerate(button_pins, start=1):
        expected = str(endpoint)
        assert (
            device.read_zigbee_attr(
                endpoint,
                ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
                ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
            )
            == expected
        )


def test_switch_cluster_attributes_preserved_via_nvm() -> None:
    """Test that writable switch cluster attributes are preserved across device restarts via NVM"""
    device_config = "A;B;SA0u;SA1u;RB0;RB1;"  # Two switches and two relays
    endpoint = 1

    test_values = {
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS: "2",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE: "1",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE: "1",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX: "2",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR: "900",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE: "100",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE: "1",
    }

    # First session: write all test values
    with StubProc(device_config=device_config) as proc:
        device = Device(proc)

        for attr_id, value in test_values.items():
            device.write_zigbee_attr(
                endpoint, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, attr_id, value
            )
    # Second session: restart device and verify values are preserved
    with StubProc(device_config=device_config) as proc:
        device = Device(proc)

        for attr_id, expected_value in test_values.items():
            actual_value = device.read_zigbee_attr(
                endpoint, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, attr_id
            )
            assert actual_value == expected_value, (
                f"Attribute {attr_id:04x} not preserved via NVM: expected {expected_value}, got {actual_value}"
            )
