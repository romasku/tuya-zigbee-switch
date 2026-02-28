"""Tests for cover switch cluster in momentary mode."""
import pytest

from client import StubProc
from conftest import Device, DEBOUNCE_MS, MINIMUM_SWITCH_TIME_MS
from zcl_consts import (
    ZCL_ATTR_COVER_SWITCH_SWITCH_TYPE,
    ZCL_ATTR_COVER_SWITCH_LONG_PRESS_DURATION,
    ZCL_ATTR_COVER_SWITCH_LOCAL_MODE,
    ZCL_ATTR_COVER_SWITCH_BINDED_MODE,
    ZCL_ATTR_COVER_SWITCH_COVER_INDEX,
    ZCL_CLUSTER_COVER_SWITCH_CONFIG,
    ZCL_COVER_SWITCH_TYPE_TOGGLE,
    ZCL_COVER_SWITCH_TYPE_MOMENTARY,
    ZCL_COVER_SWITCH_MODE_IMMEDIATE,
    ZCL_COVER_SWITCH_MODE_LONG_PRESS,
    ZCL_COVER_SWITCH_MODE_SHORT_PRESS,
    ZCL_COVER_SWITCH_MODE_HYBRID,
    ZCL_CLUSTER_WINDOW_COVERING,
    ZCL_ATTR_WINDOW_COVERING_MOVING,
    ZCL_WINDOW_COVERING_MOVING_STOPPED,
    ZCL_WINDOW_COVERING_MOVING_OPENING,
    ZCL_WINDOW_COVERING_MOVING_CLOSING,
    ZCL_CMD_WINDOW_COVERING_UP_OPEN,
    ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE,
    ZCL_CMD_WINDOW_COVERING_STOP,
)


LONG_PRESS_TIME_MS = 800


RELEASED = "0"
OPEN = "1"
CLOSE = "2"
STOP = "3"
LONG_OPEN = "4"
LONG_CLOSE = "5"


# ============================================================================
# Fixtures
# ============================================================================


@pytest.fixture
def cover_switch_device() -> Device:
    p = StubProc(device_config="Mfr;Model;XA0A1u;CB0B1;XC0C1u;CD0D1;").start()
    try:
        d = Device(p)
        # Advance time past the motor protection delay. Rather than adding extra logic to the firmware code
        # to allow immediate command execution, we handle this constraint in the test setup by advancing time here.
        d.step_time(MINIMUM_SWITCH_TIME_MS)
        yield d
    finally:
        p.stop()


@pytest.fixture
def toggle_cover_switch(cover_switch_device: Device) -> Device:
    cover_switch_device.zcl_cover_switch_set_switch_type(1, ZCL_COVER_SWITCH_TYPE_TOGGLE)
    cover_switch_device.zcl_cover_switch_set_switch_type(2, ZCL_COVER_SWITCH_TYPE_TOGGLE)
    return cover_switch_device


# ============================================================================
# Tests for toggle mode multistate value
# ============================================================================


@pytest.mark.parametrize("button,expected_state", [("A0", OPEN), ("A1", CLOSE)])
def test_toggle_button_press(toggle_cover_switch: Device, button: str, expected_state: str):
    assert toggle_cover_switch.zcl_switch_get_multistate_value(1) == STOP
    toggle_cover_switch.press_button(button)
    assert toggle_cover_switch.zcl_switch_get_multistate_value(1) == expected_state
    toggle_cover_switch.release_button(button)
    assert toggle_cover_switch.zcl_switch_get_multistate_value(1) == STOP


def test_toggle_both_buttons_pressed(toggle_cover_switch: Device):
    assert toggle_cover_switch.zcl_switch_get_multistate_value(1) == STOP
    toggle_cover_switch.press_button("A0")
    assert toggle_cover_switch.zcl_switch_get_multistate_value(1) == OPEN
    toggle_cover_switch.press_button("A1")
    assert toggle_cover_switch.zcl_switch_get_multistate_value(1) == STOP
    toggle_cover_switch.release_button("A1")
    assert toggle_cover_switch.zcl_switch_get_multistate_value(1) == OPEN
    toggle_cover_switch.release_button("A0")
    assert toggle_cover_switch.zcl_switch_get_multistate_value(1) == STOP


# ============================================================================
# Tests for momentary mode multistate value
# ============================================================================


@pytest.mark.parametrize("button,expected_state", [("A0", OPEN), ("A1", CLOSE)])
def test_momentary_button_short_press(cover_switch_device: Device, button: str, expected_state: str):
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == RELEASED
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == expected_state
    cover_switch_device.release_button(button)
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == RELEASED


def test_momentary_both_buttons_pressed(cover_switch_device: Device):
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == RELEASED
    cover_switch_device.press_button("A0")
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == OPEN
    cover_switch_device.press_button("A1")
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == STOP
    cover_switch_device.release_button("A1")
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == STOP
    cover_switch_device.release_button("A0")
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == RELEASED


@pytest.mark.parametrize("button,short_state,long_state", [("A0", OPEN, LONG_OPEN), ("A1", CLOSE, LONG_CLOSE)])
def test_momentary_button_long_press(cover_switch_device: Device, button: str, short_state: str, long_state: str):
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == RELEASED
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == short_state
    cover_switch_device.step_time(LONG_PRESS_TIME_MS)
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == long_state
    cover_switch_device.release_button(button)
    assert cover_switch_device.zcl_switch_get_multistate_value(1) == RELEASED


# ============================================================================
# Tests for stop-on-repeat behavior across different local modes
# ============================================================================


@pytest.mark.parametrize("button,direction", [("A0", ZCL_WINDOW_COVERING_MOVING_OPENING), ("A1", ZCL_WINDOW_COVERING_MOVING_CLOSING)])
def test_immediate_mode_stops_on_repeat(cover_switch_device: Device, button: str, direction: str):
    cover_switch_device.zcl_cover_switch_set_local_mode(1, ZCL_COVER_SWITCH_MODE_IMMEDIATE)
    
    # Press should start moving
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction
    cover_switch_device.release_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction
    cover_switch_device.step_time(MINIMUM_SWITCH_TIME_MS)

    # Repeated press should stop the cover
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED


@pytest.mark.parametrize("button,direction", [("A0", ZCL_WINDOW_COVERING_MOVING_OPENING), ("A1", ZCL_WINDOW_COVERING_MOVING_CLOSING)])
def test_long_press_mode_stops_on_repeat(cover_switch_device: Device, button: str, direction: str):
    cover_switch_device.zcl_cover_switch_set_local_mode(1, ZCL_COVER_SWITCH_MODE_LONG_PRESS)

    # Long press should start moving
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    cover_switch_device.step_time(LONG_PRESS_TIME_MS)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction
    cover_switch_device.release_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction

    # Repeated long press should stop the cover
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction
    cover_switch_device.step_time(LONG_PRESS_TIME_MS)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED


@pytest.mark.parametrize("button,direction", [("A0", ZCL_WINDOW_COVERING_MOVING_OPENING), ("A1", ZCL_WINDOW_COVERING_MOVING_CLOSING)])
def test_short_press_mode_stops_on_repeat(cover_switch_device: Device, button: str, direction: str):
    cover_switch_device.zcl_cover_switch_set_local_mode(1, ZCL_COVER_SWITCH_MODE_SHORT_PRESS)
    
    # Press and release should start moving
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    cover_switch_device.release_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction
    cover_switch_device.step_time(MINIMUM_SWITCH_TIME_MS)

    # Repeated press and release should stop the cover
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction
    cover_switch_device.release_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED


@pytest.mark.parametrize("button,direction", [("A0", ZCL_WINDOW_COVERING_MOVING_OPENING), ("A1", ZCL_WINDOW_COVERING_MOVING_CLOSING)])
def test_hybrid_short_press_stops_on_repeat(cover_switch_device: Device, button: str, direction: str):
    cover_switch_device.zcl_cover_switch_set_local_mode(1, ZCL_COVER_SWITCH_MODE_HYBRID)
    
    # Press and release should start moving
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    cover_switch_device.release_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction
    cover_switch_device.step_time(MINIMUM_SWITCH_TIME_MS)

    # Repeated press and release should stop the cover
    cover_switch_device.press_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == direction
    cover_switch_device.release_button(button)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED


def test_hybrid_long_keeps_moving(cover_switch_device: Device):
    cover_switch_device.zcl_cover_switch_set_local_mode(1, ZCL_COVER_SWITCH_MODE_HYBRID)
    
    # Press and release should start opening
    cover_switch_device.press_button("A0")
    cover_switch_device.release_button("A0")
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Long press in the same direction should not stop the cover
    cover_switch_device.press_button("A0")
    cover_switch_device.step_time(LONG_PRESS_TIME_MS)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Release should stop the cover
    cover_switch_device.release_button("A0")
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED


def test_hybrid_long_opposite_works(cover_switch_device: Device):
    cover_switch_device.zcl_cover_switch_set_local_mode(1, ZCL_COVER_SWITCH_MODE_HYBRID)
    
    # Press and release should start opening
    cover_switch_device.press_button("A0")
    cover_switch_device.release_button("A0")
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Long press in the opposite direction should stop the cover and start closing
    cover_switch_device.press_button("A1")
    cover_switch_device.step_time(LONG_PRESS_TIME_MS)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED

    # Wait for safety delay before changing direction
    cover_switch_device.step_time(500)
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_CLOSING
    cover_switch_device.step_time(MINIMUM_SWITCH_TIME_MS)

    # Release should stop the cover
    cover_switch_device.release_button("A1")
    assert cover_switch_device.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED


# ============================================================================
# Tests for cover index
# ============================================================================


def test_cover_switch_detached_mode(toggle_cover_switch: Device):
    # Press should start opening the cover
    toggle_cover_switch.press_button("A0")
    assert toggle_cover_switch.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_OPENING
    toggle_cover_switch.step_time(MINIMUM_SWITCH_TIME_MS)

    # Release should stop the cover
    toggle_cover_switch.release_button("A0")
    assert toggle_cover_switch.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    
    # Set cover index to 0 (detached mode)
    toggle_cover_switch.zcl_cover_switch_set_cover_index(1, 0)
    
    # Press should no longer trigger the cover
    toggle_cover_switch.press_button("A0")
    assert toggle_cover_switch.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED


def test_cover_switch_index_switching(toggle_cover_switch: Device):
    # Press should trigger the first cover endpoint
    toggle_cover_switch.press_button("A0")
    assert toggle_cover_switch.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_OPENING
    toggle_cover_switch.step_time(MINIMUM_SWITCH_TIME_MS)

    # Release should stop the cover
    toggle_cover_switch.release_button("A0")
    assert toggle_cover_switch.zcl_cover_get_moving(3) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    
    # Set cover index to 2
    toggle_cover_switch.zcl_cover_switch_set_cover_index(1, 2)

    # Press should now trigger the second cover endpoint
    toggle_cover_switch.press_button("A0")
    assert toggle_cover_switch.zcl_cover_get_moving(4) == ZCL_WINDOW_COVERING_MOVING_OPENING
    toggle_cover_switch.step_time(MINIMUM_SWITCH_TIME_MS)

    # Release should stop the cover
    toggle_cover_switch.release_button("A0")
    assert toggle_cover_switch.zcl_cover_get_moving(4) == ZCL_WINDOW_COVERING_MOVING_STOPPED


# ============================================================================
# Tests for binded device state tracking
# ============================================================================


def test_binded_stop_on_repeat(cover_switch_device: Device):
    # Disable local control
    cover_switch_device.zcl_cover_switch_set_cover_index(1, 0)

    # First press: should send UP_OPEN to binded device
    cover_switch_device.press_button("A0")
    cover_switch_device.release_button("A0")
    cover_switch_device.wait_for_cmd_send(1, ZCL_CLUSTER_WINDOW_COVERING, ZCL_CMD_WINDOW_COVERING_UP_OPEN)
    
    # Second press in same direction: should send STOP (stop-on-repeat)
    cover_switch_device.press_button("A0")
    cover_switch_device.release_button("A0")
    cover_switch_device.wait_for_cmd_send(1, ZCL_CLUSTER_WINDOW_COVERING, ZCL_CMD_WINDOW_COVERING_STOP)
    
    # Third press: should send UP_OPEN again (now stopped, so restart)
    cover_switch_device.press_button("A0")
    cover_switch_device.release_button("A0")
    cover_switch_device.wait_for_cmd_send(1, ZCL_CLUSTER_WINDOW_COVERING, ZCL_CMD_WINDOW_COVERING_UP_OPEN)


def test_binded_reverse(cover_switch_device: Device):
    # Disable local control
    cover_switch_device.zcl_cover_switch_set_cover_index(1, 0)

    # First press: should send UP_OPEN to binded device
    cover_switch_device.press_button("A0")
    cover_switch_device.release_button("A0")
    cover_switch_device.wait_for_cmd_send(1, ZCL_CLUSTER_WINDOW_COVERING, ZCL_CMD_WINDOW_COVERING_UP_OPEN)
    
    # Second press in opposite direction: should send DOWN_CLOSE to binded device
    cover_switch_device.press_button("A1")
    cover_switch_device.release_button("A1")
    cover_switch_device.wait_for_cmd_send(1, ZCL_CLUSTER_WINDOW_COVERING, ZCL_CMD_WINDOW_COVERING_DOWN_CLOSE)
