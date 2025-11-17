import pytest

from tests.conftest import Device, RelayButtonPair
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ONOFF_TOGGLE,
    ZCL_LEVEL_MOVE_DOWN,
    ZCL_LEVEL_MOVE_UP,
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
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY_NC,
)


@pytest.fixture()
def momentary_device(device: Device, relay_button_pair: RelayButtonPair) -> Device:
    device.zcl_switch_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY
    )
    return device


def test_toggle_mode_rise_mode_relay_control(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE
    )

    momentary_device.press_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1

    momentary_device.release_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1

    momentary_device.press_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.release_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0


def test_toggle_mode_short_press_mode_relay_control(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT
    )
    momentary_device.press_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.release_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1

    momentary_device.press_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1

    momentary_device.release_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.press_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.step_time(2_000)  # Long press
    momentary_device.release_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0


def test_toggle_mode_long_press_mode_relay_control(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG
    )
    momentary_device.click_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.press_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.step_time(2_000)  # Long press

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1

    momentary_device.release_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1


def test_toggle_mode_detached_mode_relay_control(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED
    )
    momentary_device.click_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.long_click_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0


# OnOff Commands Tests for Momentary Mode


@pytest.mark.parametrize(
    "actions,expected_cmd",
    [
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE, ZCL_CMD_ONOFF_TOGGLE),
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, ZCL_CMD_ONOFF_ON),
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON, ZCL_CMD_ONOFF_OFF),
    ],
    ids=["toggle", "on_off", "off_on"],
)
def test_momentary_mode_onoff_commands_rise_mode(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    actions: int,
    expected_cmd: int,
):
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.switch_endpoint,
        actions,
    )

    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, expected_cmd
    )

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)  # Wait a bit to ensure no command is sent
    assert len(momentary_device.zcl_list_cmds()) == 0


@pytest.mark.parametrize(
    "actions,expected_cmd",
    [
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE, ZCL_CMD_ONOFF_TOGGLE),
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, ZCL_CMD_ONOFF_ON),
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON, ZCL_CMD_ONOFF_OFF),
    ],
    ids=["toggle", "on_off", "off_on"],
)
def test_momentary_mode_onoff_commands_short_mode(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    actions: int,
    expected_cmd: int,
):
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.switch_endpoint,
        actions,
    )

    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds()) == 0

    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, expected_cmd
    )


@pytest.mark.parametrize(
    "actions,expected_cmd",
    [
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE, ZCL_CMD_ONOFF_TOGGLE),
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, ZCL_CMD_ONOFF_ON),
        (ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON, ZCL_CMD_ONOFF_OFF),
    ],
    ids=["toggle", "on_off", "off_on"],
)
def test_momentary_mode_onoff_commands_long_mode(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    actions: int,
    expected_cmd: int,
):
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.switch_endpoint,
        actions,
    )

    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds()) == 0

    momentary_device.release_button(relay_button_pair.button_pin)

    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds()) == 0

    momentary_device.long_click_button(relay_button_pair.button_pin)

    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, expected_cmd
    )


def test_momentary_mode_onoff_commands_smart_sync(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE
    )
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.switch_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    )

    momentary_device.zcl_relay_on(relay_button_pair.relay_endpoint)

    momentary_device.press_button(relay_button_pair.button_pin)
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds()) == 0


def test_momentary_mode_onoff_commands_smart_opposite(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE
    )
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.switch_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    )

    momentary_device.zcl_relay_on(relay_button_pair.relay_endpoint)

    momentary_device.press_button(relay_button_pair.button_pin)
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds()) == 0


# Level Control Commands Tests for Momentary Mode


def test_momentary_mode_level_control_commands_long_press(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(2_000)

    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    )

    momentary_device.release_button(relay_button_pair.button_pin)

    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    )


@pytest.mark.parametrize(
    "move_rate",
    [1, 20, 255],
)
def test_momentary_mode_level_control_commands_use_configured_step(
    momentary_device: Device, relay_button_pair: RelayButtonPair, move_rate: int
):
    momentary_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
        move_rate,
    )
    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(2_000)

    cmd_data = momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    )
    assert cmd_data.data[1] == move_rate

    momentary_device.release_button(relay_button_pair.button_pin)

    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    )


def test_momentary_mode_level_control_direction_alternates(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    # First long press - should start with one direction
    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(2_000)
    cmd_data = momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    )
    assert cmd_data.data[0] == ZCL_LEVEL_MOVE_UP
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    )
    momentary_device.clear_events()

    # Second long press - direction should alternate
    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(2_000)
    cmd_data = momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    )
    assert cmd_data.data[0] == ZCL_LEVEL_MOVE_DOWN
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    )


def test_momentary_mode_no_level_control_in_short_press(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(500)  # Short duration
    momentary_device.release_button(relay_button_pair.button_pin)

    momentary_device.step_time(100)

    assert not any(
        cmd
        for cmd in momentary_device.zcl_list_cmds()
        if cmd.cluster == ZCL_CLUSTER_LEVEL_CONTROL
    )


@pytest.mark.parametrize(
    "long_press_duration_ms",
    [500, 1000, 2000, 3000],  # Test different long press durations
)
def test_momentary_mode_long_press_duration_configuration(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    long_press_duration_ms: int,
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG
    )
    momentary_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
        long_press_duration_ms,
    )

    momentary_device.press_button(relay_button_pair.button_pin)

    momentary_device.step_time(long_press_duration_ms - 100)
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.step_time(100)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1


@pytest.mark.parametrize(
    "relay_index",
    [1, 2, 3],
)
def test_momentary_mode_relay_index_configuration(
    momentary_device: Device,
    relay_index: int,
    relay_button_pairs: list[RelayButtonPair],
):
    momentary_device.write_zigbee_attr(
        relay_button_pairs[0].switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
        relay_index,
    )

    momentary_device.click_button(relay_button_pairs[0].button_pin)

    target_pair = relay_button_pairs[relay_index - 1]

    assert momentary_device.zcl_relay_get(target_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(target_pair.relay_pin) == 1

    momentary_device.click_button(relay_button_pairs[0].button_pin)

    assert momentary_device.zcl_relay_get(target_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(target_pair.relay_pin) == 0


@pytest.mark.parametrize(
    "invalid_relay_index",
    [0, 100, 255],
)
def test_momentary_mode_relay_index_invalid_values(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    invalid_relay_index: int,
):
    momentary_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
        invalid_relay_index,
    )

    momentary_device.click_button(relay_button_pair.button_pin)

    momentary_device.step_time(100)


# Test multistate state


def test_momentary_mode_multistate_value(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    assert (
        momentary_device.zcl_switch_get_multistate_value(
            relay_button_pair.switch_endpoint
        )
        == "0"
    )

    momentary_device.press_button(relay_button_pair.button_pin)

    assert (
        momentary_device.zcl_switch_get_multistate_value(
            relay_button_pair.switch_endpoint
        )
        == "1"
    )
    momentary_device.step_time(2_000)  # Long press
    assert (
        momentary_device.zcl_switch_get_multistate_value(
            relay_button_pair.switch_endpoint
        )
        == "2"
    )

    momentary_device.release_button(relay_button_pair.button_pin)

    assert (
        momentary_device.zcl_switch_get_multistate_value(
            relay_button_pair.switch_endpoint
        )
        == "0"
    )


def test_momentary_mode_multistate_value_normaly_closed(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_mode_set(
        relay_button_pair.switch_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY_NC,
    )

    # Do one cycle to allow button to get to correct state
    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.release_button(relay_button_pair.button_pin)

    assert (
        momentary_device.zcl_switch_get_multistate_value(
            relay_button_pair.switch_endpoint
        )
        == "1"
    )

    momentary_device.step_time(2_000)  # Long press
    assert (
        momentary_device.zcl_switch_get_multistate_value(
            relay_button_pair.switch_endpoint
        )
        == "2"
    )

    momentary_device.press_button(relay_button_pair.button_pin)

    assert (
        momentary_device.zcl_switch_get_multistate_value(
            relay_button_pair.switch_endpoint
        )
        == "0"
    )


def test_momentary_mode_multistate_action_reporting(
        momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.press_button(relay_button_pair.button_pin)
    assert (
        momentary_device.zcl_switch_await_multistate_value_reported_change(relay_button_pair.switch_endpoint)
        == "1"
    )

    momentary_device.step_time(2_000)
    assert (
        momentary_device.zcl_switch_await_multistate_value_reported_change(relay_button_pair.switch_endpoint)
        == "2"
    )

    momentary_device.release_button(relay_button_pair.button_pin)
    assert (
        momentary_device.zcl_switch_await_multistate_value_reported_change(relay_button_pair.switch_endpoint)
        == "0"
    )
