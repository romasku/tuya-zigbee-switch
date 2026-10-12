import pytest

from tests.conftest import Device, RelayButtonPair
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
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


def test_toggle_mode_long_press_ep_silent(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    """Toggle mode early-returns before invoking long-press: long_press_ep
    fires on long-press unconditionally in Momentary mode, but here must stay
    fully silent."""
    toggle_device.press_button(relay_button_pair.button_pin)
    toggle_device.step_time(2_000)
    assert len(toggle_device.zcl_list_cmds(
        endpoint=relay_button_pair.long_press_endpoint
    )) == 0

    toggle_device.release_button(relay_button_pair.button_pin)


def test_toggle_mode_long_press_ep_relay_mode_long_silent(
    toggle_device: Device, relay_button_pair: RelayButtonPair
):
    """Toggle mode early-returns before invoking long-press: long_press_ep
    `relay_mode=LONG` must not retoggle the relay during the hold."""
    toggle_device.zcl_switch_relay_mode_set(
        relay_button_pair.long_press_endpoint,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    toggle_device.press_button(relay_button_pair.button_pin)
    relay_after_press = toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint)
    toggle_device.step_time(2_000)
    assert toggle_device.zcl_relay_get(relay_button_pair.relay_endpoint) == relay_after_press
    toggle_device.release_button(relay_button_pair.button_pin)
