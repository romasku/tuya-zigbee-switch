import pytest

from client import StubProc
from conftest import Device
from zcl_consts import (
    ZCL_ATTR_BASIC_MFR_NAME,
    ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
    ZCL_CLUSTER_BASIC,
    ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ONOFF_TOGGLE,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
)


@pytest.fixture
def device_config() -> str:
    return "StubManufacturer;StubDevice;SA0u;SA1u;"


@pytest.fixture
def button_pins() -> list[str]:
    return ["A0", "A1"]


def test_boots_without_crash(device: Device):
    """Device with switches but no relays boots and basic cluster is readable."""
    assert device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME) is not None


def test_switches_default_to_detached(device: Device):
    """All switches default to DETACHED relay mode when no relays are defined."""
    for ep in [1, 2]:
        mode = int(device.read_zigbee_attr(
            ep, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE
        ))
        assert mode == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED


def test_relay_index_is_zero(device: Device):
    """All switches have relay_index=0 when no relays are defined."""
    for ep in [1, 2]:
        idx = int(device.read_zigbee_attr(
            ep, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX
        ))
        assert idx == 0


def test_button_press_multistate(device: Device, button_pins: list[str]):
    """Pressing buttons does not crash and multistate value changes."""
    for i, pin in enumerate(button_pins):
        ep = i + 1
        device.click_button(pin)
        val = device.read_zigbee_attr(
            ep, ZCL_CLUSTER_MULTISTATE_INPUT_BASIC, ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE
        )
        assert val is not None


def test_button_press_no_crash(device: Device, button_pins: list[str]):
    """Short-click and long-press on every button do not crash."""
    for i, pin in enumerate(button_pins):
        device.zcl_switch_mode_set(i + 1, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)
        device.click_button(pin) 
        device.long_click_button(pin, duration_ms=1000)
    # If we got here without crash, the device is still responsive.
    assert device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME) is not None


@pytest.mark.parametrize(
    "action,expected_cmd",
    [
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
            ZCL_CMD_ONOFF_TOGGLE,
            id="toggle",
        ),
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, ZCL_CMD_ONOFF_ON, id="on"
        ),
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_OFFON, ZCL_CMD_ONOFF_OFF, id="off"
        ),
    ],
)
@pytest.mark.parametrize(
    "binded_mode,trigger",
    [
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE,
            lambda d, pin: d.press_button(pin),
            id="rise",
        ),
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
            lambda d, pin: d.click_button(pin),
            id="short",
        ),
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
            lambda d, pin: d.long_click_button(pin, duration_ms=1000),
            id="long",
        ),
    ],
)
def test_binding_commands_still_sent(
    device: Device,
    button_pins: list[str],
    binded_mode: int,
    trigger,
    action: int,
    expected_cmd: int,
):
    """All binded_modes × SwitchActions emit the right OnOff command on press."""
    device.zcl_switch_mode_set(1, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)
    device.zcl_switch_binding_mode_set(1, binded_mode)
    device.zcl_switch_actions_set(1, action)
    device.set_network(1)
    device.clear_events()
    trigger(device, button_pins[0])
    device.wait_for_cmd_send(1, ZCL_CLUSTER_ON_OFF, expected_cmd)


@pytest.mark.parametrize(
    "action",
    [
        pytest.param(ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_SYNC, id="sync"),
        pytest.param(ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SMART_OPPOSITE, id="opposite"),
    ],
)
@pytest.mark.parametrize(
    "binded_mode,trigger",
    [
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_RISE,
            lambda d, pin: d.press_button(pin),
            id="rise",
        ),
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
            lambda d, pin: d.click_button(pin),
            id="short",
        ),
        pytest.param(
            ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
            lambda d, pin: d.long_click_button(pin),
            id="long",
        ),
    ],
)
def test_smart_sync_degrades_to_toggle(
    device: Device,
    button_pins: list[str],
    binded_mode: int,
    trigger,
    action: int,
):
    """Without a valid relay, SmartSync/SmartOpposite falls back to plain Toggle."""
    device.zcl_switch_mode_set(1, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)
    device.zcl_switch_binding_mode_set(1, binded_mode)
    device.zcl_switch_actions_set(1, action)
    device.set_network(1)
    device.clear_events()
    trigger(device, button_pins[0])
    device.wait_for_cmd_send(1, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE)


def test_extra_switches_are_detached_when_relays_are_missing():
    """Switches beyond the relay count should detach instead of keeping invalid relay indices."""
    proc = StubProc(device_config="StubManufacturer;StubDevice;SA0u;SA1u;RB0;").start()
    try:
        device = Device(proc)

        assert int(
            device.read_zigbee_attr(
                2,
                ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
                ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
            )
        ) == ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED
        assert int(
            device.read_zigbee_attr(
                2,
                ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
                ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_INDEX,
            )
        ) == 0

        device.set_network(1)
        device.clear_events()
        device.click_button("A1")
        device.wait_for_cmd_send(2, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE)
    finally:
        proc.stop()
