"""Tests for simultaneous operation of switch_ep and long_press_ep.

When switch_ep uses deprecated LONG modes (relay_mode or binded_mode), the
long_press_ep is muted to keep behavior identical to the legacy single-endpoint
firmware — no double relay toggles, no conflicting emissions.
"""

import pytest

from tests.conftest import Device, RelayButtonPair
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
)


@pytest.fixture()
def momentary_device(device: Device, relay_button_pair: RelayButtonPair) -> Device:
    device.zcl_switch_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY
    )
    return device


@pytest.mark.parametrize(
    "switch_ep_relay_mode,switch_ep_binded_mode,expected_relay_value",
    [
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
            "1",
        ),
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
            "0",
        ),
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
            "1",
        ),
    ],
    ids=["relay_long", "binded_long", "both_long"],
)
def test_switch_ep_long_mode_mutes_long_press_ep_for_same_relay(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    switch_ep_relay_mode: int,
    switch_ep_binded_mode: int,
    expected_relay_value: str,
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, switch_ep_relay_mode
    )
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, switch_ep_binded_mode
    )
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )

    momentary_device.long_click_button(relay_button_pair.button_pin)

    # If long_press_ep flipped, the relay would settle at the opposite value.
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == expected_relay_value


@pytest.mark.parametrize(
    "switch_ep_relay_mode,switch_ep_binded_mode",
    [
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
        ),
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
        ),
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
        ),
    ],
    ids=["relay_long", "binded_long", "both_long"],
)
def test_switch_ep_long_mode_mutes_long_press_ep_for_other_relay(
    momentary_device: Device,
    relay_button_pairs: list[RelayButtonPair],
    switch_ep_relay_mode: int,
    switch_ep_binded_mode: int,
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pairs[0].switch_endpoint, switch_ep_relay_mode
    )
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pairs[0].switch_endpoint, switch_ep_binded_mode
    )
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    momentary_device.write_zigbee_attr(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
        2,
    )

    momentary_device.long_click_button(relay_button_pairs[0].button_pin)

    # long_press_ep is fully muted — the relay it points to stays untouched.
    assert momentary_device.zcl_relay_get(relay_button_pairs[1].relay_endpoint) == "0"


@pytest.mark.parametrize(
    "switch_ep_relay_mode,switch_ep_binded_mode",
    [
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
        ),
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
        ),
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
        ),
    ],
    ids=["relay_long", "binded_long", "both_long"],
)
def test_switch_ep_long_mode_mutes_long_press_ep_onoff_emit(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    switch_ep_relay_mode: int,
    switch_ep_binded_mode: int,
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, switch_ep_relay_mode
    )
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, switch_ep_binded_mode
    )

    momentary_device.long_click_button(relay_button_pair.button_pin)

    # long_press_ep is fully muted — no OnOff emission from it.
    assert len(
        momentary_device.zcl_list_cmds(
            endpoint=relay_button_pair.long_press_endpoint,
            cluster=ZCL_CLUSTER_ON_OFF,
        )
    ) == 0


@pytest.mark.parametrize(
    "switch_ep_relay_mode,switch_ep_binded_mode",
    [
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
        ),
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
        ),
        (
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
        ),
    ],
    ids=["relay_long", "binded_long", "both_long"],
)
def test_switch_ep_long_mode_mutes_long_press_ep_level_emit(
    momentary_device: Device,
    relay_button_pair: RelayButtonPair,
    switch_ep_relay_mode: int,
    switch_ep_binded_mode: int,
):
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, switch_ep_relay_mode
    )
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, switch_ep_binded_mode
    )

    momentary_device.long_click_button(relay_button_pair.button_pin)

    # long_press_ep is fully muted — no Level Control emission from it
    # (switch_ep may still emit on its own endpoint; only long_press_ep is checked).
    assert len(
        momentary_device.zcl_list_cmds(
            endpoint=relay_button_pair.long_press_endpoint,
            cluster=ZCL_CLUSTER_LEVEL_CONTROL,
        )
    ) == 0


def test_switch_ep_rise_does_not_mute_long_press_ep_relay_control(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    """switch_ep in non-LONG mode (RISE here) leaves long_press_ep active —
    the same relay flips twice: once on press, once on long-press."""
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE
    )
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )

    momentary_device.press_button(relay_button_pair.button_pin)
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"

    momentary_device.step_time(2_000)  # Long press
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"

    momentary_device.release_button(relay_button_pair.button_pin)
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"


def test_switch_ep_rise_and_long_press_ep_long_flip_different_relays(
    momentary_device: Device, relay_button_pairs: list[RelayButtonPair]
):
    """switch_ep RISE and long_press_ep LONG target different relays — each
    one flips its own relay at its own moment."""
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pairs[0].switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE
    )
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    momentary_device.write_zigbee_attr(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
        2,
    )

    momentary_device.press_button(relay_button_pairs[0].button_pin)
    assert momentary_device.zcl_relay_get(relay_button_pairs[0].relay_endpoint) == "1"
    assert momentary_device.zcl_relay_get(relay_button_pairs[1].relay_endpoint) == "0"

    momentary_device.step_time(2_000)  # Long press
    assert momentary_device.zcl_relay_get(relay_button_pairs[0].relay_endpoint) == "1"
    assert momentary_device.zcl_relay_get(relay_button_pairs[1].relay_endpoint) == "1"

    momentary_device.release_button(relay_button_pairs[0].button_pin)
    assert momentary_device.zcl_relay_get(relay_button_pairs[0].relay_endpoint) == "1"
    assert momentary_device.zcl_relay_get(relay_button_pairs[1].relay_endpoint) == "1"


def test_switch_ep_rise_smart_sync_does_not_mute_long_press_ep_smart_sync(
    momentary_device: Device, relay_button_pair: RelayButtonPair
):
    """switch_ep RISE+SmartSync and long_press_ep LONG+SmartSync share a relay.
    Each emits its own OnOff command at its own moment, mirroring the
    post-flip relay state."""
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE
    )
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pair.switch_endpoint, ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.switch_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    )

    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    )

    momentary_device.press_button(relay_button_pair.button_pin)
    # switch_ep RISE flips the relay to "1", then SmartSync mirrors → ON.
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"
    momentary_device.wait_for_cmd_send(
        relay_button_pair.switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )

    momentary_device.clear_events()
    momentary_device.step_time(2_000)  # Long press
    # long_press_ep LONG flips it back to "0", SmartSync mirrors → OFF.
    assert momentary_device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"
    momentary_device.wait_for_cmd_send(
        relay_button_pair.long_press_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pair.button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds(cluster=ZCL_CLUSTER_ON_OFF)) == 0


def test_switch_ep_rise_smart_sync_and_long_press_ep_long_smart_sync_different_relays(
    momentary_device: Device, relay_button_pairs: list[RelayButtonPair]
):
    """switch_ep RISE+SmartSync targets its own relay; long_press_ep LONG+
    SmartSync targets another. Each emits ON mirroring its own post-flip relay."""
    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pairs[0].switch_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE,
    )
    momentary_device.zcl_switch_binding_mode_set(
        relay_button_pairs[0].switch_endpoint,
        ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE,
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pairs[0].switch_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    )

    momentary_device.zcl_switch_relay_mode_set(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    momentary_device.zcl_switch_actions_set(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    )
    momentary_device.write_zigbee_attr(
        relay_button_pairs[0].long_press_endpoint,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
        2,
    )

    momentary_device.press_button(relay_button_pairs[0].button_pin)
    # switch_ep flips relay_1 to "1", SmartSync mirrors → ON. relay_2 untouched.
    assert momentary_device.zcl_relay_get(relay_button_pairs[0].relay_endpoint) == "1"
    assert momentary_device.zcl_relay_get(relay_button_pairs[1].relay_endpoint) == "0"
    momentary_device.wait_for_cmd_send(
        relay_button_pairs[0].switch_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )

    momentary_device.clear_events()
    momentary_device.step_time(2_000)  # Long press
    # long_press_ep flips relay_2 to "1", SmartSync mirrors → ON. relay_1 stays "1".
    assert momentary_device.zcl_relay_get(relay_button_pairs[0].relay_endpoint) == "1"
    assert momentary_device.zcl_relay_get(relay_button_pairs[1].relay_endpoint) == "1"
    momentary_device.wait_for_cmd_send(
        relay_button_pairs[0].long_press_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )

    momentary_device.clear_events()
    momentary_device.release_button(relay_button_pairs[0].button_pin)
    momentary_device.step_time(100)
    assert len(momentary_device.zcl_list_cmds(cluster=ZCL_CLUSTER_ON_OFF)) == 0
