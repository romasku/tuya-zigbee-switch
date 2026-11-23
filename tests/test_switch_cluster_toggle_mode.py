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
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT,
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


@pytest.mark.parametrize(
    "pause_ms",
    [
        pytest.param(0, id="short"),
        pytest.param(1_000, id="long"),
    ],
)
@pytest.mark.parametrize(
    "binded_mode",
    [
        pytest.param(ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE, id="rise"),
        pytest.param(ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT, id="short"),
        pytest.param(ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG, id="long"),
    ],
)
def test_toggle_mode_toggle_actions_commands(
    toggle_device: Device,
    relay_button_pair: RelayButtonPair,
    binded_mode: int,
    pause_ms: int,
):
    """Toggle mode is edges-only: action=TOGGLE_SIMPLE emits Toggle on press
    and on release, nothing in between, regardless of binded_mode (a Momentary-
    only setting) and hold duration."""
    toggle_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    )
    toggle_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, binded_mode
    )

    toggle_device.press_button(relay_button_pair.button_pin)

    toggle_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE
    )

    # Hold for any duration — no extra emit.
    toggle_device.clear_events()
    toggle_device.step_time(pause_ms)
    assert toggle_device.zcl_list_cmds(cluster=ZCL_CLUSTER_ON_OFF) == []

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


@pytest.mark.parametrize(
    "pause_ms",
    [
        pytest.param(0, id="short"),
        pytest.param(1_000, id="long"),
    ],
)
@pytest.mark.parametrize(
    "relay_mode",
    [
        pytest.param(ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE, id="rise"),
        pytest.param(ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT, id="short"),
        pytest.param(ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG, id="long"),
    ],
)
def test_toggle_mode_relay_mode_flips_only_on_edges(
    toggle_device: Device,
    relay_button_pair: RelayButtonPair,
    relay_mode: int,
    pause_ms: int,
):
    """Toggle mode is edges-only: relay flips on press and on release, nothing
    in between, regardless of relay_mode (a Momentary-only setting) and hold
    duration."""
    toggle_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, relay_mode
    )

    toggle_device.press_button(relay_button_pair.button_pin)
    relay_after_press = toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint)

    # Hold for any duration — relay stays put.
    toggle_device.step_time(pause_ms)
    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == relay_after_press

    # Release flips back.
    toggle_device.release_button(relay_button_pair.button_pin)
    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) != relay_after_press


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
