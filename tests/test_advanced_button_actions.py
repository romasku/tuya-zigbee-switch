"""
Tests for the multi-press reporting system (2-timer state machine).

Architecture:
  - timer_hold  fires at long_press_duration_ms after press  → hold event (value persists during hold)
  - timer_confirm fires at confirm_release_ms after release  → N-press event (value persists until next press)
  - Backward compatibility: relay_mode / binded_mode still work unchanged

Multistate values:
  0  NOT_PRESSED
  1  PRESS (transient, on button down)
  2  LONG_PRESS = N=1 hold  (backward compat)
  3  POSITION_ON  (toggle mode)
  4  POSITION_OFF (toggle mode)
  5  single_press   (timer_confirm, N=1)
  6  single_release (released after hold, N=1)
  7  double_press   (timer_confirm, N=2)
  8  double_hold    (timer_hold, N=2)
  9  double_release (released after hold, N=2)
  10 triple_press   (timer_confirm, N=3)
  11 triple_hold    (timer_hold, N=3)
  12 triple_release (released after hold, N=3)
"""
import pytest

from tests.conftest import Device, RelayButtonPair, StubProc
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_CONFIRM_RELEASE_DUR,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MAX_PRESS_COUNT,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    ZCL_CMD_ONOFF_TOGGLE,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
)

# Timing helpers
HOLD_STEP_MS = 1_000   # > default long_press_duration_ms (800 ms)
CONFIRM_STEP_MS = 300  # > default confirm_release_ms (200 ms)


@pytest.fixture()
def momentary_detached(device: Device, relay_button_pair: RelayButtonPair) -> Device:
    """Momentary switch, relay and binding both detached."""
    ep = relay_button_pair.switch_endpoint
    device.zcl_switch_mode_set(ep, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)
    device.zcl_switch_relay_mode_set(ep, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED)
    device.zcl_switch_binding_mode_set(ep, 0)  # DETACHED
    return device


# ---------------------------------------------------------------------------
# Press multistate values (reported after timer_confirm fires)
# ---------------------------------------------------------------------------


def test_single_press_multistate_is_5(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """Single click reports multistate=5 (single_press) after confirm timer."""
    ep = relay_button_pair.switch_endpoint
    momentary_detached.click_button(relay_button_pair.button_pin)
    momentary_detached.step_time(CONFIRM_STEP_MS)
    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "5"


def test_double_press_multistate_is_7(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """Double click reports multistate=7 (double_press) after confirm timer."""
    ep = relay_button_pair.switch_endpoint
    momentary_detached.click_button(relay_button_pair.button_pin)
    momentary_detached.click_button(relay_button_pair.button_pin)
    momentary_detached.step_time(CONFIRM_STEP_MS)
    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "7"


def test_triple_press_multistate_is_10(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """Triple click with max_press_count=3 reports multistate=10 (triple_press)."""
    ep = relay_button_pair.switch_endpoint
    momentary_detached.zcl_switch_max_press_count_set(ep, 3)
    momentary_detached.click_button(relay_button_pair.button_pin)
    momentary_detached.click_button(relay_button_pair.button_pin)
    momentary_detached.click_button(relay_button_pair.button_pin)
    momentary_detached.step_time(CONFIRM_STEP_MS)
    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "10"


# ---------------------------------------------------------------------------
# Hold multistate values (persists during hold)
# ---------------------------------------------------------------------------


def test_hold_n1_multistate_is_2(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """N=1 hold must keep multistate == 2 (MULTISTATE_LONG_PRESS) for backward compat."""
    ep = relay_button_pair.switch_endpoint
    momentary_detached.press_button(relay_button_pair.button_pin)
    momentary_detached.step_time(HOLD_STEP_MS)

    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "2"

    momentary_detached.release_button(relay_button_pair.button_pin)
    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "0"


def test_hold_n2_multistate_is_8(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """N=2 hold must produce multistate == 8 (double_hold)."""
    ep = relay_button_pair.switch_endpoint

    # First click (release before timer_hold fires)
    momentary_detached.press_button(relay_button_pair.button_pin)
    momentary_detached.release_button(relay_button_pair.button_pin)
    # Second press — hold
    momentary_detached.press_button(relay_button_pair.button_pin)
    momentary_detached.step_time(HOLD_STEP_MS)

    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "8"

    momentary_detached.release_button(relay_button_pair.button_pin)
    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "0"


# ---------------------------------------------------------------------------
# max_press_count
# ---------------------------------------------------------------------------


def test_max_press_count_clamps_n(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """max_press_count=1: double click still fires single_press (multistate=5)."""
    ep = relay_button_pair.switch_endpoint
    momentary_detached.zcl_switch_max_press_count_set(ep, 1)

    # Double click — but max_press_count=1 clamps n to 1
    momentary_detached.click_button(relay_button_pair.button_pin)
    momentary_detached.click_button(relay_button_pair.button_pin)
    momentary_detached.step_time(CONFIRM_STEP_MS)

    # Should report single_press (5), not double_press (7)
    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "5"


def test_max_press_count_readable_as_attr(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """max_press_count attribute can be read back after write."""
    ep = relay_button_pair.switch_endpoint
    momentary_detached.zcl_switch_max_press_count_set(ep, 3)
    assert momentary_detached.read_zigbee_attr(
        ep,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MAX_PRESS_COUNT,
    ) == "3"


# ---------------------------------------------------------------------------
# confirm_release_dur
# ---------------------------------------------------------------------------


def test_confirm_release_dur_short(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """confirm_release_dur=100ms: single_press reported after 100ms, not after 200ms."""
    ep = relay_button_pair.switch_endpoint
    momentary_detached.zcl_switch_confirm_release_dur_set(ep, 100)

    momentary_detached.click_button(relay_button_pair.button_pin)
    # 50ms: confirm timer (100ms) has NOT fired yet
    momentary_detached.step_time(50)
    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "0"

    # 150ms total: confirm timer has fired
    momentary_detached.step_time(100)
    assert momentary_detached.zcl_switch_get_multistate_value(ep) == "5"


def test_confirm_release_dur_readable_as_attr(
    momentary_detached: Device, relay_button_pair: RelayButtonPair
):
    """confirm_release_dur attribute can be read back after write."""
    ep = relay_button_pair.switch_endpoint
    momentary_detached.zcl_switch_confirm_release_dur_set(ep, 350)
    assert momentary_detached.read_zigbee_attr(
        ep,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_CONFIRM_RELEASE_DUR,
    ) == "350"


# ---------------------------------------------------------------------------
# Backward compatibility: legacy relay_mode / binded_mode unchanged
# ---------------------------------------------------------------------------


def test_backward_compat_short_mode_fires_immediately(
    device: Device, relay_button_pair: RelayButtonPair
):
    """relay_mode=SHORT still fires on release without waiting for timer_confirm."""
    ep = relay_button_pair.switch_endpoint
    device.zcl_switch_mode_set(ep, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)
    device.zcl_switch_relay_mode_set(ep, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT)

    device.press_button(relay_button_pair.button_pin)
    assert device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"

    device.release_button(relay_button_pair.button_pin)
    # relay fires immediately on release (no waiting for confirm timer)
    assert device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"


def test_backward_compat_long_mode_fires_on_hold(
    device: Device, relay_button_pair: RelayButtonPair
):
    """relay_mode=LONG still fires when hold timer fires (unchanged from before)."""
    ep = relay_button_pair.switch_endpoint
    device.zcl_switch_mode_set(ep, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)
    device.zcl_switch_relay_mode_set(ep, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG)

    device.press_button(relay_button_pair.button_pin)
    assert device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"

    device.step_time(HOLD_STEP_MS)
    assert device.zcl_relay_get(relay_button_pair.relay_endpoint) == "1"

    device.release_button(relay_button_pair.button_pin)


def test_backward_compat_level_control_fires_on_long_press(
    device: Device, relay_button_pair: RelayButtonPair
):
    """Legacy level_control fires on long press (binded_mode=SHORT)."""
    ep = relay_button_pair.switch_endpoint
    device.zcl_switch_mode_set(ep, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)

    device.press_button(relay_button_pair.button_pin)
    device.step_time(HOLD_STEP_MS)

    device.wait_for_cmd_send(ep, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF)

    device.release_button(relay_button_pair.button_pin)

    device.wait_for_cmd_send(ep, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STOP_WITH_ON_OFF)


def test_backward_compat_multistate_long_press_still_2(
    device: Device, relay_button_pair: RelayButtonPair
):
    """Multistate value must still be 2 during long press (backward compat for Z2M converters)."""
    ep = relay_button_pair.switch_endpoint
    device.zcl_switch_mode_set(ep, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)

    device.press_button(relay_button_pair.button_pin)
    device.step_time(HOLD_STEP_MS)

    assert device.zcl_switch_get_multistate_value(ep) == "2"

    device.release_button(relay_button_pair.button_pin)
    assert device.zcl_switch_get_multistate_value(ep) == "0"


# ---------------------------------------------------------------------------
# NVM persistence of v2 attributes
# ---------------------------------------------------------------------------


def test_nvm_persist_v2_attributes() -> None:
    """confirm_release_ms and max_press_count must survive a device restart via NVM."""
    device_config = "A;B;SA0u;RB0;"
    endpoint = 1

    test_attrs = {
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_CONFIRM_RELEASE_DUR: "150",
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MAX_PRESS_COUNT: "3",
    }

    with StubProc(device_config=device_config) as proc:
        d = Device(proc)
        for attr_id, value in test_attrs.items():
            d.write_zigbee_attr(endpoint, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, attr_id, value)

    with StubProc(device_config=device_config) as proc:
        d = Device(proc)
        for attr_id, expected in test_attrs.items():
            actual = d.read_zigbee_attr(endpoint, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, attr_id)
            assert actual == expected, (
                f"Attribute 0x{attr_id:04x} not preserved: expected {expected}, got {actual}"
            )
