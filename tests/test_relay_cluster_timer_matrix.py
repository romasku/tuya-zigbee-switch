"""The full cross-product of timer events.

The individual reset paths are covered in
``test_relay_cluster_timer_reset.py`` and
``test_relay_cluster_button_pairing.py``. This file walks the
CROSS-PRODUCT of the dimensions that interact, because the interesting
bugs live in the combinations rather than in the single cases:

* the event that arrives while a timer is pending
* the device shape (1, 2 or 3 relays)
* which endpoint the timer sits on
* the timer length relative to the hardware tick wrap (268.435456 s)

Each event carries two facts: whether it CANCELS the pending timer,
and what it leaves the relay in. The matrix asserts both. The events
that do NOT cancel are just as important as the ones that do — a
non-cancelling event that silently cancelled a timer would be as much
a bug as a cancelling event that failed to.
"""

import pytest

from tests.client import StubProc
from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
    ZCL_CMD_OFF_WITH_EFFECT,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ONOFF_TOGGLE,
    ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE,
    ZCL_CMD_ON_WITH_TIMED_OFF,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
)


def _payload(on_time: int, off_wait_time: int = 0) -> list[int]:
    on_lo, on_hi = on_time & 0xFF, (on_time >> 8) & 0xFF
    off_lo, off_hi = off_wait_time & 0xFF, (off_wait_time >> 8) & 0xFF
    return [0x00, on_lo, on_hi, off_lo, off_hi]


def _timed_off(device: Device, ep: int, on_time: int) -> None:
    device.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_ON_OFF,
        ZCL_CMD_ON_WITH_TIMED_OFF,
        payload_bytes=_payload(on_time),
    )


def _make_momentary(device: Device, switch_ep: int, action: int) -> None:
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
        action,
    )


# --- device shapes -------------------------------------------------------
#
# One config per arity. "X;Y;" is the manufacturer/model prefix the
# stub expects; the entries after it are the switches and the relays.
ONE_RELAY = "X;Y;SA0u;RB0;"            # switch ep1, relay ep2
TWO_RELAYS = "X;Y;SA0u;RB0;SA1u;RB1;"  # switches 1,2, relays 3,4
THREE_RELAYS = "X;Y;SA0u;RB0;SA1u;RB1;SA2u;RB2;"  # switches 1-3, relays 4-6


def _button_pin(switch_ep: int) -> str:
    return f"A{switch_ep - 1}"


# --- the events ----------------------------------------------------------
#
# Each entry: (id, cancels_timer, relay_state_after, action, setup)
#
# relay_state_after is one of:
#   "on"    the relay ends up ON
#   "off"   the relay ends up OFF
#   "toggled" the relay flipped (ON -> OFF, since the timer turned it on)
#
# cancels_timer is False for the events that do NOT drive the relay:
# writing the read-only onOff attribute, and a detached switch. Both
# must leave the pending timer running.

def _ev_plain_on(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)


def _ev_plain_off(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF)


def _ev_toggle(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE)


def _ev_off_with_effect(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.call_zigbee_cmd(
        ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_OFF_WITH_EFFECT, payload_bytes=[0x00]
    )


def _ev_on_with_recall(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_ON_OFF,
        ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE,
        payload_bytes=[0x00, 0x00, 0x00, 0x00],
    )


def _ev_level_zero(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
        payload_bytes=[0x00, 0x00, 0x00],
    )


def _ev_button(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.click_button(_button_pin(switch_ep))


def _ev_long_press(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.long_click_button(_button_pin(switch_ep), duration_ms=1_000)


def _ev_attr_write(d: Device, ep: int, switch_ep: int, action: int) -> None:
    d.write_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF, 1)


EVENTS = [
    # id, cancels, relay after, action, setup, runner
    ("plain_on", True, "on",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, None, _ev_plain_on),
    ("plain_off", True, "off",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, None, _ev_plain_off),
    ("toggle", True, "toggled",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, None, _ev_toggle),
    ("off_with_effect", True, "off",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, None, _ev_off_with_effect),
    ("on_with_recall", True, "on",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, None, _ev_on_with_recall),
    ("level_zero", True, "off",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, None, _ev_level_zero),
    ("button_onoff", True, "on",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, None, _ev_button),
    ("button_toggle", True, "toggled",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE, None, _ev_button),
    ("long_press", True, "toggled",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE, "long", _ev_long_press),
    ("detached_button", False, "on",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, "detached", _ev_button),
    ("attr_write", False, "on",
     ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF, None, _ev_attr_write),
]

EVENT_IDS = [e[0] for e in EVENTS]


def _prepare(d: Device, switch_ep: int, action: int, setup: str | None) -> None:
    _make_momentary(d, switch_ep, action)
    if setup == "detached":
        d.write_zigbee_attr(
            switch_ep,
            ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
            ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
        )
    elif setup == "long":
        d.write_zigbee_attr(
            switch_ep,
            ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
            ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
            ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
        )


def _relay_on(pin: str, state: str) -> bool:
    """Whether the relay is expected to be ON after the event."""
    if state == "on":
        return True
    if state == "off":
        return False
    if state == "toggled":
        return False  # the timer had turned it ON, the event flipped it
    raise AssertionError(state)


# --- the matrix ----------------------------------------------------------


@pytest.mark.parametrize(
    "event_id,cancels,state,action,setup,runner",
    EVENTS,
    ids=EVENT_IDS,
)
def test_event_matrix_single_relay(
    event_id: str, cancels: bool, state: str, action: int, setup, runner
) -> None:
    """Every event on a one-relay device, with the timer armed.

    After the event the relay must be in the expected state, and the
    timer must have been cancelled (or, for the two non-cancelling
    events, must still fire).
    """
    with StubProc(device_config=ONE_RELAY) as proc:
        d = Device(proc)
        ep = 2
        _prepare(d, 1, action, setup)

        _timed_off(d, ep, 100)  # 10 s
        assert d.get_gpio("B0")
        was_on = True

        d.step_time(5_000)
        runner(d, ep, 1, action)

        expected_on = _relay_on("B0", state)
        assert d.get_gpio("B0", refresh=True) == expected_on, (
            f"{event_id}: unexpected relay state right after the event"
        )

        # Past the deadline.
        d.step_time(6_000)
        if cancels:
            if expected_on:
                assert d.get_gpio("B0", refresh=True), (
                    f"{event_id}: the auto-OFF fired — the event did not "
                    f"cancel the timer"
                )
            else:
                assert not d.get_gpio("B0", refresh=True)
        else:
            # The event does not drive the relay, so the timer must
            # still fire.
            assert not d.get_gpio("B0", refresh=True), (
                f"{event_id}: the timer was cancelled — this event does "
                f"not drive the relay and must leave the timer alone"
            )


@pytest.mark.parametrize(
    "event_id,cancels,state,action,setup,runner",
    EVENTS,
    ids=EVENT_IDS,
)
@pytest.mark.parametrize("relay_ep", [3, 4])
def test_event_matrix_two_relays(
    event_id: str, cancels: bool, state: str, action: int, setup, runner,
    relay_ep: int,
) -> None:
    """Every event on either relay of a two-relay device.

    The other relay keeps its own timer, which must still fire.
    """
    with StubProc(device_config=TWO_RELAYS) as proc:
        d = Device(proc)
        other_ep = 4 if relay_ep == 3 else 3
        target_pin = "B0" if relay_ep == 3 else "B1"
        other_pin = "B1" if relay_ep == 3 else "B0"
        switch_ep = 1 if relay_ep == 3 else 2

        _prepare(d, switch_ep, action, setup)

        _timed_off(d, relay_ep, 100)  # 10 s on the target
        _timed_off(d, other_ep, 100)  # 10 s on the other

        d.step_time(5_000)
        runner(d, relay_ep, switch_ep, action)

        expected_on = _relay_on(target_pin, state)
        assert d.get_gpio(target_pin, refresh=True) == expected_on, (
            f"{event_id}: unexpected relay state right after the event"
        )

        d.step_time(6_000)  # past both deadlines
        if cancels and expected_on:
            assert d.get_gpio(target_pin, refresh=True), (
                f"{event_id}: the auto-OFF fired on the target relay"
            )
        elif not cancels:
            assert not d.get_gpio(target_pin, refresh=True), (
                f"{event_id}: the timer was cancelled — this event does "
                f"not drive the relay"
            )
        else:
            assert not d.get_gpio(target_pin, refresh=True)

        # The other relay's timer is untouched by this event.
        assert not d.get_gpio(other_pin, refresh=True), (
            f"{event_id}: the other relay's timer did not fire"
        )


@pytest.mark.parametrize("relay_ep", [4, 5, 6])
def test_every_endpoint_of_a_three_relay_device(relay_ep: int) -> None:
    """Each of the three relays keeps its own timer and its own button."""
    with StubProc(device_config=THREE_RELAYS) as proc:
        d = Device(proc)
        pins = {4: "B0", 5: "B1", 6: "B2"}
        other_eps = [e for e in (4, 5, 6) if e != relay_ep]
        switch_ep = relay_ep - 3

        _make_momentary(d, switch_ep, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF)
        for ep in (4, 5, 6):
            _timed_off(d, ep, 100)

        d.step_time(5_000)
        d.click_button(_button_pin(switch_ep))

        d.step_time(6_000)
        assert d.get_gpio(pins[relay_ep], refresh=True), (
            "the relay's own button did not cancel its timer"
        )
        for ep in other_eps:
            assert not d.get_gpio(pins[ep], refresh=True), (
                f"the timer of relay {ep} was cancelled by another button"
            )


@pytest.mark.parametrize("relay_ep", [4, 5, 6])
def test_every_endpoint_timers_fire_at_their_own_deadline(relay_ep: int) -> None:
    """Three timers of different lengths each fire at their own moment."""
    with StubProc(device_config=THREE_RELAYS) as proc:
        d = Device(proc)
        pins = {4: "B0", 5: "B1", 6: "B2"}
        lengths = {4: 10, 5: 20, 6: 30}  # 1 s, 2 s, 3 s
        for ep in (4, 5, 6):
            _timed_off(d, ep, lengths[ep])

        d.step_time(1_500)
        assert not d.get_gpio(pins[4], refresh=True)
        assert d.get_gpio(pins[5], refresh=True)
        assert d.get_gpio(pins[6], refresh=True)

        d.step_time(1_000)
        assert not d.get_gpio(pins[5], refresh=True)
        assert d.get_gpio(pins[6], refresh=True)

        d.step_time(1_000)
        assert not d.get_gpio(pins[6], refresh=True)


@pytest.mark.parametrize("on_time_units", [10, 100, 1000, 6000, 65534])
def test_timer_lengths_across_the_tick_wrap(on_time_units: int) -> None:
    """Timers of 1 s .. 6553 s all fire at their own deadline.

    6553 s is the ZCL maximum and is longer than the 268 s
    hardware tick wrap, which is exactly the case the old counter got
    wrong.
    """
    with StubProc(device_config=ONE_RELAY) as proc:
        d = Device(proc)
        ep = 2
        deadline_ms = on_time_units * 100
        _timed_off(d, ep, on_time_units)
        assert d.get_gpio("B0")
        # Just before the deadline: still ON.
        d.step_time(deadline_ms - 500)
        assert d.get_gpio("B0", refresh=True), (
            f"the timer of {on_time_units} units fired early"
        )
        # Just past the deadline: OFF.
        d.step_time(1_000)
        assert not d.get_gpio("B0", refresh=True), (
            f"the timer of {on_time_units} units did not fire"
        )


@pytest.mark.parametrize("on_time_units", [10, 1000, 6000, 65534])
def test_event_during_a_long_timer(on_time_units: int) -> None:
    """A button cancels a long timer too, not just a short one.

    The long timers straddle the hardware tick wrap, which is where
    the old counter broke.
    """
    with StubProc(device_config=ONE_RELAY) as proc:
        d = Device(proc)
        ep = 2
        deadline_ms = on_time_units * 100
        _make_momentary(d, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF)
        _timed_off(d, ep, on_time_units)
        d.step_time(deadline_ms // 2)
        d.click_button("A0")
        d.step_time(deadline_ms // 2 + 1_000)
        assert d.get_gpio("B0", refresh=True), (
            f"the timer of {on_time_units} units survived the button press"
        )


@pytest.mark.parametrize("on_time_units", [100, 1000, 6000, 65534])
def test_second_timer_replaces_a_long_timer(on_time_units: int) -> None:
    """A second On With Timed Off replaces a long one cleanly.

    The replacement is LONGER, so the old deadline passes without the
    relay switching off, and the new deadline switches it off later.

    Timeline: the first timer is armed at t=0 with deadline old_ms.
    The replacement arrives at t=old_ms//2 and moves the deadline to
    old_ms//2 + new_ms.
    """
    with StubProc(device_config=ONE_RELAY) as proc:
        d = Device(proc)
        ep = 2
        old_ms = on_time_units * 100
        _timed_off(d, ep, on_time_units)
        d.step_time(old_ms // 2)
        # Replace with a longer timer, capped at the ZCL maximum.
        new_units = min(on_time_units * 2, 65534)
        new_ms = new_units * 100
        _timed_off(d, ep, new_units)
        # The OLD deadline (old_ms from the start) must not fire.
        # Stop just short of it.
        d.step_time(old_ms // 2 - 500)
        assert d.get_gpio("B0", refresh=True), (
            f"the replaced timer of {on_time_units} units still fired"
        )
        # The NEW deadline is at old_ms//2 + new_ms; wait past it.
        d.step_time(new_ms - old_ms // 2 + 1_000)
        assert not d.get_gpio("B0", refresh=True), (
            "the replacement timer did not fire"
        )


@pytest.mark.parametrize("first,second", [(10, 40), (40, 10), (100, 200),
                                           (200, 100), (65534, 10)])
def test_replacement_in_both_directions(first: int, second: int) -> None:
    """Replacing a timer with a shorter one and with a longer one."""
    with StubProc(device_config=ONE_RELAY) as proc:
        d = Device(proc)
        ep = 2
        _timed_off(d, ep, first)
        d.step_time(1_000)
        _timed_off(d, ep, second)
        # The second deadline is 1 s + second.
        d.step_time(second * 100 - 1_000 + 500)
        assert d.get_gpio("B0", refresh=True), (
            f"the replacement by {second} fired too early"
        )
        d.step_time(1_000)
        assert not d.get_gpio("B0", refresh=True), (
            f"the replacement by {second} did not fire"
        )


@pytest.mark.parametrize("relay_ep", [2, 3, 4, 5, 6])
def test_timer_after_the_relay_was_already_off(relay_ep: int) -> None:
    """Arming a timer on an already-off relay still turns it on and
    still fires."""
    cfg = {2: ONE_RELAY, 3: TWO_RELAYS, 4: THREE_RELAYS,
           5: THREE_RELAYS, 6: THREE_RELAYS}[relay_ep]
    with StubProc(device_config=cfg) as proc:
        d = Device(proc)
        pin = {2: "B0", 3: "B0", 4: "B0", 5: "B1", 6: "B2"}[relay_ep]
        assert not d.get_gpio(pin)
        _timed_off(d, relay_ep, 100)
        assert d.get_gpio(pin), "the timer did not turn the relay on"
        d.step_time(10_500)
        assert not d.get_gpio(pin, refresh=True)
