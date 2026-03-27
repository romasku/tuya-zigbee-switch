import pytest

from tests.conftest import DEBOUNCE_MS, Device, RelayButtonPair
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ONOFF_TOGGLE,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE,
)


@pytest.fixture()
def toggle_device(device: Device, relay_button_pair: RelayButtonPair) -> Device:
    device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
        ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE,
    )
    return device


@pytest.mark.skipif(DEBOUNCE_MS == 0, reason="No software debounce configured")
def test_toggle_mode_no_reaction_before_debounce(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    toggle_device.set_gpio(relay_button_pair.button_pin, 0)  # Low is pressed
    toggle_device.step_time(DEBOUNCE_MS - 1)

    assert (
        toggle_device.read_zigbee_attr(
            relay_button_pair.relay_endpoint,
            ZCL_CLUSTER_ON_OFF,
            ZCL_ATTR_ONOFF,
        )
        == "0"
    )
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 0


@pytest.mark.parametrize(
    "actions",
    [
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE,
    ],
    ids=["toggle", "smart_sync", "smart_opposite"],
)
def test_toggle_mode_toggle_actions_relay_control(
    toggle_device: Device, relay_button_pair: RelayButtonPair, actions: int
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        actions,
    )
    toggle_device.press_button(relay_button_pair.button_pin)

    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 1

    toggle_device.release_button(relay_button_pair.button_pin)
    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 0

    toggle_device.zcl_relay_on(relay_button_pair.relay_endpoint)
    toggle_device.press_button(relay_button_pair.button_pin)

    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 0


def test_toggle_mode_onoff_actions_relay_control(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    )
    toggle_device.press_button(relay_button_pair.button_pin)

    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 1

    toggle_device.release_button(relay_button_pair.button_pin)
    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 0

    toggle_device.zcl_relay_on(relay_button_pair.relay_endpoint)
    toggle_device.press_button(relay_button_pair.button_pin)

    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 1


def test_toggle_mode_offon_actions_relay_control(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON,
    )
    toggle_device.press_button(relay_button_pair.button_pin)

    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 0

    toggle_device.release_button(relay_button_pair.button_pin)
    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 1

    toggle_device.zcl_relay_off(relay_button_pair.relay_endpoint)
    toggle_device.press_button(relay_button_pair.button_pin)

    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 0


def test_toggle_mode_toggle_actions_commands(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    )

    toggle_device.press_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE
    )

    toggle_device.release_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE
    )


def test_toggle_mode_onoff_actions_commands(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    )

    toggle_device.press_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )

    toggle_device.release_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )


def test_toggle_mode_offon_actions_commands(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON,
    )

    toggle_device.press_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )

    toggle_device.release_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )


def test_toggle_mode_smart_sync_actions_commands(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    )

    toggle_device.zcl_relay_on(relay_button_pair.relay_endpoint)
    toggle_device.press_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )

    toggle_device.zcl_relay_on(relay_button_pair.relay_endpoint)

    toggle_device.release_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )


def test_toggle_mode_smart_opposite_actions_commands(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE,
    )

    toggle_device.zcl_relay_on(relay_button_pair.relay_endpoint)

    toggle_device.press_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )

    toggle_device.zcl_relay_on(relay_button_pair.relay_endpoint)

    toggle_device.release_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )


def test_relay_not_controlled_if_detached(
    toggle_device: Device,
    relay_button_pair: RelayButtonPair,
):
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    )
    toggle_device.press_button(relay_button_pair.button_pin)

    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 0

    toggle_device.release_button(relay_button_pair.button_pin)
    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 0

    toggle_device.zcl_relay_on(relay_button_pair.relay_endpoint)
    toggle_device.press_button(relay_button_pair.button_pin)

    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 1

    toggle_device.release_button(relay_button_pair.button_pin)
    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert toggle_device.get_gpio(relay_button_pair.relay_pin) == 1


# Test multistate state


def test_toggle_mode_multistate_value(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    assert (
        toggle_device.zcl_switch_get_multistate_value(relay_button_pair.switch_endpoint)
        == "4"
    )

    toggle_device.press_button(relay_button_pair.button_pin)

    assert (
        toggle_device.zcl_switch_get_multistate_value(relay_button_pair.switch_endpoint)
        == "3"
    )

    toggle_device.release_button(relay_button_pair.button_pin)

    assert (
        toggle_device.zcl_switch_get_multistate_value(relay_button_pair.switch_endpoint)
        == "4"
    )
