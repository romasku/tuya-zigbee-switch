"""Which button cancels which relay's timer when the wiring is crossed.

The switch-to-relay pairing comes from two per-type counters in
``config_parser.c``:

    switch_clusters[n].relay_index = n + 1;    (n = index of the Nth 'S' entry)
    relay_clusters[m].relay       = &relays[m]; (m = index of the Nth 'R' entry)

So the Nth 'S' entry drives the Nth 'R' entry — in the ORDER OF
APPEARANCE WITHIN EACH TYPE, not in the order the entries sit in the
config string, and not by which pin is physically nearest.

That is what makes a crossed wiring possible at all. A two-gang board
whose left button is wired to the right relay is expressed as

    SA0u;RB1;SA1u;RB0;

— the 'R' entries are listed in the opposite order to the 'S' entries.
Switch 1 (pin A0) then drives relay 1 (pin B1, endpoint 3) and switch 2
(pin A1) drives relay 2 (pin B0, endpoint 4).

The rule for the timer is simple and does not change with the wiring:
**a button cancels the timer of the relay it drives, and of no other.**
The timer belongs to the relay endpoint; the button only reaches it
through the relay it is paired with.

These tests pin both halves down: the pairing under a crossed wiring,
and the fact that the button cancels its own relay's timer while
leaving the other one armed.
"""

import pytest

from tests.client import StubProc
from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_ON_WITH_TIMED_OFF,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
)


def _payload(on_time: int, off_wait_time: int = 0) -> list[int]:
    on_lo, on_hi = on_time & 0xFF, (on_time >> 8) & 0xFF
    off_lo, off_hi = off_wait_time & 0xFF, (off_wait_time >> 8) & 0xFF
    return [0x00, on_lo, on_hi, off_lo, off_hi]


def _on_with_timed_off(device: Device, ep: int, on_time: int) -> None:
    device.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_ON_OFF,
        ZCL_CMD_ON_WITH_TIMED_OFF,
        payload_bytes=_payload(on_time),
    )


def _make_momentary(device: Device, switch_ep: int) -> None:
    """A momentary switch whose press drives the relay ON.

    ONOFF action is used on purpose: it drives the relay without
    changing its state, so the relay stays ON and a surviving timer
    would switch it OFF at its deadline.
    """
    device.write_zigbee_attr(
        switch_ep,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
        ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
    )
    device.write_zigbee_attr(
        switch_ep,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
        ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    )


# Crossed wiring: switch 1 (pin A0) -> relay 1 (pin B1), switch 2
# (pin A1) -> relay 2 (pin B0). The 'R' entries are in the opposite
# order to the 'S' entries.
CROSSED_CFG = "X;Y;SA0u;RB1;SA1u;RB0;"
# Straight wiring, for comparison: switch 1 -> relay on B0.
STRAIGHT_CFG = "X;Y;SA0u;RB0;SA1u;RB1;"


@pytest.fixture()
def crossed() -> Device:
    with StubProc(device_config=CROSSED_CFG) as proc:
        yield Device(proc)


@pytest.fixture()
def straight() -> Device:
    with StubProc(device_config=STRAIGHT_CFG) as proc:
        yield Device(proc)


# --- the pairing under a crossed wiring ----------------------------------


def test_crossed_switch1_drives_relay1(crossed: Device) -> None:
    """With the 'R' entries reversed, switch 1 drives the relay on pin B1.

    The relay endpoints follow the 'R' order, so the relay on pin B1
    is endpoint 3 and the one on pin B0 is endpoint 4.
    """
    _make_momentary(crossed, 1)
    _on_with_timed_off(crossed, 3, 100)  # 10 s on endpoint 3
    assert crossed.get_gpio("B1")
    assert not crossed.get_gpio("B0")
    crossed.click_button("A0")
    assert crossed.get_gpio("B1", refresh=True)
    assert not crossed.get_gpio("B0", refresh=True), (
        "switch 1 drove the relay on B0 — the pairing is not by the "
        "order of the 'R' entries"
    )


def test_crossed_switch2_drives_relay2(crossed: Device) -> None:
    """Switch 2 drives the relay on pin B0, which is endpoint 4."""
    _make_momentary(crossed, 2)
    _on_with_timed_off(crossed, 4, 100)  # 10 s on endpoint 4
    assert crossed.get_gpio("B0")
    crossed.click_button("A1")
    assert crossed.get_gpio("B0", refresh=True)
    assert not crossed.get_gpio("B1", refresh=True), (
        "switch 2 drove the relay on B1 — the pairing is not by the "
        "order of the 'R' entries"
    )


def test_straight_switch1_drives_relay_on_b0(straight: Device) -> None:
    """The straight wiring pairs switch 1 with the relay on pin B0."""
    _make_momentary(straight, 1)
    _on_with_timed_off(straight, 3, 100)
    assert straight.get_gpio("B0")
    straight.click_button("A0")
    assert straight.get_gpio("B0", refresh=True)
    assert not straight.get_gpio("B1", refresh=True)


# --- which timer does the button cancel under a crossed wiring ------------


def test_crossed_button1_cancels_relay1_timer(crossed: Device) -> None:
    """Switch 1 cancels the timer of the relay it drives (endpoint 3)."""
    _make_momentary(crossed, 1)
    _on_with_timed_off(crossed, 3, 100)  # 10 s
    crossed.step_time(5_000)
    crossed.click_button("A0")
    crossed.step_time(6_000)  # past the deadline
    assert crossed.get_gpio("B1", refresh=True), (
        "switch 1 did not cancel the timer of the relay it drives"
    )


def test_crossed_button1_leaves_relay2_timer_running(crossed: Device) -> None:
    """Switch 1 must not cancel the timer of the OTHER relay."""
    _make_momentary(crossed, 1)
    _on_with_timed_off(crossed, 4, 100)  # 10 s on the relay switch 1 does NOT drive
    crossed.step_time(5_000)
    crossed.click_button("A0")
    crossed.step_time(6_000)
    assert not crossed.get_gpio("B0", refresh=True), (
        "switch 1 cancelled the timer of the relay it does not drive"
    )


def test_crossed_button2_cancels_relay2_timer(crossed: Device) -> None:
    """Switch 2 cancels the timer of the relay it drives (endpoint 4)."""
    _make_momentary(crossed, 2)
    _on_with_timed_off(crossed, 4, 100)  # 10 s
    crossed.step_time(5_000)
    crossed.click_button("A1")
    crossed.step_time(6_000)
    assert crossed.get_gpio("B0", refresh=True), (
        "switch 2 did not cancel the timer of the relay it drives"
    )


def test_crossed_button2_leaves_relay1_timer_running(crossed: Device) -> None:
    """Switch 2 must not cancel the timer of the OTHER relay."""
    _make_momentary(crossed, 2)
    _on_with_timed_off(crossed, 3, 100)  # 10 s on the relay switch 2 does NOT drive
    crossed.step_time(5_000)
    crossed.click_button("A1")
    crossed.step_time(6_000)
    assert not crossed.get_gpio("B1", refresh=True), (
        "switch 2 cancelled the timer of the relay it does not drive"
    )


def test_crossed_both_buttons_cancel_their_own_timers(crossed: Device) -> None:
    """Each button cancels its own relay's timer, and the other timer
    still fires."""
    _make_momentary(crossed, 1)
    _make_momentary(crossed, 2)
    _on_with_timed_off(crossed, 3, 100)  # 10 s, relay on B1
    _on_with_timed_off(crossed, 4, 100)  # 10 s, relay on B0

    crossed.step_time(5_000)
    crossed.click_button("A0")  # cancels the relay on B1
    crossed.click_button("A1")  # cancels the relay on B0

    crossed.step_time(6_000)
    assert crossed.get_gpio("B1", refresh=True)
    assert crossed.get_gpio("B0", refresh=True), (
        "a button cancelled the timer of the relay it does not drive"
    )


def test_crossed_timers_are_per_relay_endpoint(crossed: Device) -> None:
    """Under a crossed wiring the timers still belong to the relay
    endpoints, so each one fires at its own deadline."""
    _make_momentary(crossed, 1)
    _make_momentary(crossed, 2)
    _on_with_timed_off(crossed, 3, 10)  # 1 s, relay on B1
    _on_with_timed_off(crossed, 4, 40)  # 4 s, relay on B0

    crossed.step_time(1_500)
    assert not crossed.get_gpio("B1", refresh=True), (
        "the relay on B1 did not switch off at its own 1 s deadline"
    )
    assert crossed.get_gpio("B0", refresh=True)

    crossed.step_time(3_000)
    assert not crossed.get_gpio("B0", refresh=True), (
        "the relay on B0 did not switch off at its own 4 s deadline"
    )
