import pytest

from tests.conftest import Device, RelayButtonPair
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_DIRECTION,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MOVE_COMMAND,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_LEVEL_MOVE,
    ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    ZCL_CMD_LEVEL_STOP,
    ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ONOFF_TOGGLE,
    ZCL_LEVEL_MOVE_DIRECTION_ALTERNATE,
    ZCL_LEVEL_MOVE_DOWN,
    ZCL_LEVEL_MOVE_UP,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
)


@pytest.fixture()
def momentary_device(device: Device, relay_button_pair: RelayButtonPair) -> Device:
    device.zcl_switch_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY
    )
    device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED
    )
    return device


def test_long_press_ep_momentary_mode_long_press_mode_relay_control(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )

    momentary_device.press_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 0

    momentary_device.step_time(2_000)  # Long press

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1

    momentary_device.release_button(relay_button_pair.button_pin)

    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(relay_button_pair.relay_pin) == 1


def test_long_press_ep_momentary_mode_detached_mode_relay_control(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
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
def test_long_press_ep_momentary_mode_onoff_commands(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    actions: int,
    expected_cmd: int,
):
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.long_press_endpoint,
        actions,
    )

    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds()) == 0

    momentary_device.step_time(2_000)  # Long press

    momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint, ZCL_CLUSTER_ON_OFF, expected_cmd
    )

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds(cluster=ZCL_CLUSTER_ON_OFF)) == 0


def test_long_press_ep_momentary_mode_onoff_commands_smart_sync(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    )

    momentary_device.zcl_relay_on(relay_button_pair.relay_endpoint)

    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert len(momentary_device.zcl_list_cmds()) == 0

    momentary_device.step_time(2_000)  # Long press

    # relay_mode=LONG flips the local relay to "0", then SmartSync reads the
    # post-flip state and emits OFF to keep bound devices in sync.
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds(cluster=ZCL_CLUSTER_ON_OFF)) == 0


def test_long_press_ep_momentary_mode_onoff_commands_smart_opposite(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE,
    )

    momentary_device.zcl_relay_on(relay_button_pair.relay_endpoint)

    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    assert len(momentary_device.zcl_list_cmds()) == 0

    momentary_device.step_time(2_000)  # Long press

    # relay_mode=LONG flips the local relay to "0", then SmartOpposite reads
    # the post-flip state and emits ON (opposite of local relay) to bindings.
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds(cluster=ZCL_CLUSTER_ON_OFF)) == 0


# Level Control Commands Tests for Momentary Mode


@pytest.mark.parametrize(
    "move_command,expected_move_cmd,expected_stop_cmd",
    [
        (
            ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
            ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
            ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
        ),
        (
            ZCL_CMD_LEVEL_MOVE,
            ZCL_CMD_LEVEL_MOVE,
            ZCL_CMD_LEVEL_STOP,
        ),
    ],
    ids=["move_with_onoff", "move"],
)
def test_long_press_ep_momentary_mode_level_control_commands(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    move_command: int,
    expected_move_cmd: int,
    expected_stop_cmd: int,
):
    momentary_device.write_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MOVE_COMMAND,
        move_command,
    )

    momentary_device.press_button(relay_button_pair.button_pin)
    momentary_device.step_time(500)  # Short duration
    assert len(
        momentary_device.zcl_list_cmds(
            endpoint=relay_button_pair.long_press_endpoint,
            cluster=ZCL_CLUSTER_LEVEL_CONTROL,
        )
    ) == 0

    momentary_device.step_time(2_000)  # Long press

    momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        expected_move_cmd,
    )
    assert len(
        momentary_device.zcl_list_cmds(
            endpoint=relay_button_pair.long_press_endpoint,
            cluster=ZCL_CLUSTER_LEVEL_CONTROL,
        )
    ) == 1

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pair.button_pin)

    momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        expected_stop_cmd,
    )
    assert len(
        momentary_device.zcl_list_cmds(
            endpoint=relay_button_pair.long_press_endpoint,
            cluster=ZCL_CLUSTER_LEVEL_CONTROL,
        )
    ) == 1


@pytest.mark.parametrize(
    "move_rate",
    [1, 20, 255],
)
def test_long_press_ep_momentary_mode_level_control_commands_use_configured_step(
    momentary_device: Device, relay_button_pair: RelayButtonPair, move_rate: int
):
    momentary_device.write_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_RATE,
        move_rate,
    )

    momentary_device.long_click_button(relay_button_pair.button_pin)

    cmd_data = momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    )
    assert cmd_data.data[1] == move_rate


def test_long_press_ep_momentary_mode_level_control_direction_alternates(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.write_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_DIRECTION,
        ZCL_LEVEL_MOVE_DIRECTION_ALTERNATE,
    )

    momentary_device.long_click_button(relay_button_pair.button_pin)
    cmd_data = momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    )
    assert cmd_data.data[0] == ZCL_LEVEL_MOVE_UP

    momentary_device.clear_events()

    momentary_device.long_click_button(relay_button_pair.button_pin)
    cmd_data = momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    )
    assert cmd_data.data[0] == ZCL_LEVEL_MOVE_DOWN


@pytest.mark.parametrize(
    "direction",
    [ZCL_LEVEL_MOVE_UP, ZCL_LEVEL_MOVE_DOWN],
    ids=["up", "down"],
)
def test_long_press_ep_momentary_mode_level_control_direction_constant(
    momentary_device: Device, relay_button_pair: RelayButtonPair, direction: int
):
    momentary_device.write_zigbee_attr(
        relay_button_pair.long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LEVEL_MOVE_DIRECTION,
        direction,
    )

    # Two long presses in a row keep the same direction (no alternation).
    for _ in range(2):
        momentary_device.long_click_button(relay_button_pair.button_pin)
        cmd_data = momentary_device.wait_for_cmd_send(
            relay_button_pair.long_press_endpoint,
            ZCL_CLUSTER_LEVEL_CONTROL,
            ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
        )
        assert cmd_data.data[0] == direction
        momentary_device.clear_events()


@pytest.mark.parametrize(
    "long_press_duration_ms",
    [500, 1000, 2000, 3000],
)
def test_long_press_ep_momentary_mode_long_press_duration_configuration(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    long_press_duration_ms: int,
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
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


def test_long_press_ep_momentary_mode_long_press_duration_zero_doesnt_freeze_device(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    momentary_device.write_zigbee_attr(
        relay_button_pair.switch_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_LONG_PRESS_DUR,
        0,
    )

    momentary_device.press_button(relay_button_pair.button_pin)

    # Just to check device is responsive
    momentary_device.get_gpio(relay_button_pair.relay_pin, refresh=True)


@pytest.mark.parametrize(
    "relay_index",
    [1, 2, 3],
)
def test_long_press_ep_momentary_mode_relay_index_configuration(
    momentary_device: Device,
    relay_index: int,
    relay_button_pairs: list[RelayButtonPair],
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    momentary_device.write_zigbee_attr(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
        relay_index,
    )

    momentary_device.long_click_button(relay_button_pairs[0].button_pin)

    target_pair = relay_button_pairs[relay_index - 1]

    assert momentary_device.zcl_relay_get(target_pair.relay_endpoint) == "1"
    assert momentary_device.get_gpio(target_pair.relay_pin) == 1

    momentary_device.long_click_button(relay_button_pairs[0].button_pin)

    assert momentary_device.zcl_relay_get(target_pair.relay_endpoint) == "0"
    assert momentary_device.get_gpio(target_pair.relay_pin) == 0
