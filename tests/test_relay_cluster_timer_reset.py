"""Every path that can cancel or replace a pending auto-OFF timer.

The ZCL "On With Timed Off" timer is per relay cluster (per endpoint).
This file walks each way the pending deadline can be cleared or moved
and asserts the relay ends up in the state the user would expect.

The paths covered here:

* plain ON / OFF / TOGGLE commands
* ON with recall global scene, OFF with effect
* level control move-to-level
* a second On With Timed Off (must REPLACE, not stack or cancel)
* the physical button (short press, long press)
* the button of the *other* endpoint (must not touch this timer)
* a plain attribute write (must not cancel)
* the timer firing on its own

The stub REPL runs with time frozen by default; `step_time` advances the
virtual clock so deadlines can be evaluated deterministically.
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
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
)


def _payload(on_time: int, off_wait_time: int) -> list[int]:
    """The 5-byte onWithTimedOff payload, little-endian onTime/offWaitTime."""
    on_lo, on_hi = on_time & 0xFF, (on_time >> 8) & 0xFF
    off_lo, off_hi = off_wait_time & 0xFF, (off_wait_time >> 8) & 0xFF
    return [0x00, on_lo, on_hi, off_lo, off_hi]


def _on_with_timed_off(
    device: Device, ep: int, on_time: int, off_wait_time: int = 0
) -> None:
    device.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_ON_OFF,
        ZCL_CMD_ON_WITH_TIMED_OFF,
        payload_bytes=_payload(on_time, off_wait_time),
    )


def _onoff_reports(device: Device, ep: int) -> int:
    """Count the onOff attribute reports seen so far on an endpoint.

    The stub's reader thread delivers the EVT lines asynchronously, so
    an event emitted by a command may still be in flight when the
    command's reply arrives. `status` is a round trip that lets the
    reader catch up before the count is taken.
    """
    device.status()
    count = 0
    for e in device._events:
        if e.kind != "zcl_attr_change":
            continue
        if int(e.payload["ep"]) != ep:
            continue
        if int(e.payload["cluster"], 16) != ZCL_CLUSTER_ON_OFF:
            continue
        if int(e.payload["attr"], 16) != ZCL_ATTR_ONOFF:
            continue
        count += 1
    return count


def _make_momentary(device: Device, switch_ep: int, action: int) -> None:
    """Turn the switch into a momentary button with the given action.

    The device config defaults to TOGGLE switch type, where a press and
    a release each drive the relay — two toggles for one click. A
    momentary switch drives the relay once, on the release, which is
    what a real push button does.
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
        action,
    )


@pytest.fixture()
def device() -> Device:
    # One button + one relay: switch endpoint 1, relay endpoint 2.
    cfg = "X;Y;SA0u;RB0;"
    with StubProc(device_config=cfg) as proc:
        yield Device(proc)


@pytest.fixture()
def two_relay_device() -> Device:
    # Two buttons + two relays: switch_eps=1,2; relay_eps=3,4.
    cfg = "X;Y;SA0u;SA1u;RB0;RB1;"
    with StubProc(device_config=cfg) as proc:
        yield Device(proc)


@pytest.fixture()
def three_relay_device() -> Device:
    # Three buttons + three relays: switch_eps=1,2,3; relay_eps=4,5,6.
    cfg = "X;Y;SA0u;SA1u;SA2u;RB0;RB1;RB2;"
    with StubProc(device_config=cfg) as proc:
        yield Device(proc)


# --- a single-channel device ---------------------------------------------
#
# The real single-channel boards (MODULE_AVATTO_A_TS0001) have one switch
# and one relay: config "4rbqgcuv;TS0001-AVB;BC2u;LD2i;SD3u;RC0;".
# The relay endpoint is 2 there. The two-channel boards
# (MODULE_AVATTO_TS0012) have two of each: "ljhbw1c9;TS0012-avatto;
# BB4u;LB5;SC0u;RC2;SC3u;RC4;" — switches on endpoints 1,2 and relays
# on endpoints 3,4.


def test_single_channel_device_timer_fires(device: Device) -> None:
    """A one-relay device arms and fires its timer normally."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    assert device.get_gpio("B0")
    device.step_time(10_500)
    assert not device.get_gpio("B0", refresh=True)
    assert device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) == "0"


def test_single_channel_device_button_cancels_timer(device: Device) -> None:
    """On a one-relay device the button cancels the pending timer."""
    ep = 2
    _make_momentary(device, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF)
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.click_button("A0")
    device.step_time(6_000)
    assert device.get_gpio("B0", refresh=True), (
        "auto-OFF fired after a button press on a single-channel device"
    )


# --- both channels of a two-channel device --------------------------------


def test_both_channels_arm_and_fire_independently(
    two_relay_device: Device,
) -> None:
    """Both relays of a two-channel device run their own timer.

    Channel 1 gets a 1 s timer, channel 2 a 4 s one. Channel 1 must
    switch off first and channel 2 later — each at its own deadline.
    """
    ep_a, ep_b = 3, 4
    _on_with_timed_off(two_relay_device, ep_a, on_time=10)  # 1 s
    _on_with_timed_off(two_relay_device, ep_b, on_time=40)  # 4 s
    assert two_relay_device.get_gpio("B0")
    assert two_relay_device.get_gpio("B1")

    two_relay_device.step_time(1_500)
    assert not two_relay_device.get_gpio("B0", refresh=True), (
        "channel 1 did not switch off at its own 1 s deadline"
    )
    assert two_relay_device.get_gpio("B1", refresh=True), (
        "channel 2 switched off early — it followed the other channel"
    )

    two_relay_device.step_time(3_000)
    assert not two_relay_device.get_gpio("B1", refresh=True), (
        "channel 2 did not switch off at its own 4 s deadline"
    )


def test_channel1_button_cancels_only_channel1_timer(
    two_relay_device: Device,
) -> None:
    """The button of channel 1 cancels channel 1's timer and leaves
    channel 2's timer running."""
    ep_a, ep_b = 3, 4
    _make_momentary(
        two_relay_device, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF
    )
    _on_with_timed_off(two_relay_device, ep_a, on_time=100)  # 10 s
    _on_with_timed_off(two_relay_device, ep_b, on_time=100)  # 10 s

    two_relay_device.step_time(5_000)
    two_relay_device.click_button("A0")  # channel 1's button

    two_relay_device.step_time(6_000)  # past both deadlines
    # Channel 1's timer was cancelled, so its relay is still ON.
    assert two_relay_device.get_gpio("B0", refresh=True), (
        "channel 1's timer was not cancelled by its own button"
    )
    # Channel 2's timer was untouched, so it fired.
    assert not two_relay_device.get_gpio("B1", refresh=True), (
        "channel 2's timer was cancelled by the other channel's button"
    )


def test_detached_button_does_not_cancel_the_timer(device: Device) -> None:
    """A detached switch does not drive the relay.

    The relay therefore keeps no trace of the button press, the pending
    auto-OFF survives, and the relay still switches off on time. This
    is the documented behaviour of the detach mode: the button is not
    connected to the relay at all.
    """
    ep = 2
    _make_momentary(device, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF)
    device.write_zigbee_attr(
        1,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    )
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    assert device.get_gpio("B0")

    device.click_button("A0")
    # The relay did not move and the timer is still armed.
    assert device.get_gpio("B0", refresh=True)

    device.step_time(10_500)
    assert not device.get_gpio("B0", refresh=True), (
        "the timer did not fire — a detached button must not cancel it"
    )


def test_detached_button_of_channel1_leaves_channel2_timer_running(
    two_relay_device: Device,
) -> None:
    """A detached switch on channel 1 leaves both timers running."""
    ep_a, ep_b = 3, 4
    _make_momentary(
        two_relay_device, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF
    )
    two_relay_device.write_zigbee_attr(
        1,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    )
    _on_with_timed_off(two_relay_device, ep_a, on_time=100)  # 10 s
    _on_with_timed_off(two_relay_device, ep_b, on_time=100)  # 10 s

    two_relay_device.click_button("A0")
    two_relay_device.step_time(10_500)
    assert not two_relay_device.get_gpio("B0", refresh=True)
    assert not two_relay_device.get_gpio("B1", refresh=True)


def test_three_channels_each_keep_their_own_timer(
    three_relay_device: Device,
) -> None:
    """A three-relay device keeps three independent timers."""
    ep_a, ep_b, ep_c = 4, 5, 6
    _on_with_timed_off(three_relay_device, ep_a, on_time=10)  # 1 s
    _on_with_timed_off(three_relay_device, ep_b, on_time=20)  # 2 s
    _on_with_timed_off(three_relay_device, ep_c, on_time=30)  # 3 s

    three_relay_device.step_time(1_500)
    assert not three_relay_device.get_gpio("B0", refresh=True)
    assert three_relay_device.get_gpio("B1", refresh=True)
    assert three_relay_device.get_gpio("B2", refresh=True)

    three_relay_device.step_time(1_000)
    assert not three_relay_device.get_gpio("B1", refresh=True)
    assert three_relay_device.get_gpio("B2", refresh=True)

    three_relay_device.step_time(1_000)
    assert not three_relay_device.get_gpio("B2", refresh=True)


def test_cancel_on_one_channel_does_not_affect_the_others(
    three_relay_device: Device,
) -> None:
    """Cancelling one channel's timer leaves the other two armed."""
    ep_a, ep_b, ep_c = 4, 5, 6
    _on_with_timed_off(three_relay_device, ep_a, on_time=100)  # 10 s
    _on_with_timed_off(three_relay_device, ep_b, on_time=100)  # 10 s
    _on_with_timed_off(three_relay_device, ep_c, on_time=100)  # 10 s

    three_relay_device.step_time(5_000)
    # Cancel only channel 2.
    three_relay_device.call_zigbee_cmd(ep_b, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    three_relay_device.step_time(6_000)

    assert not three_relay_device.get_gpio("B0", refresh=True), (
        "channel 1's timer did not fire"
    )
    assert three_relay_device.get_gpio("B1", refresh=True), (
        "channel 2's timer was not cancelled"
    )
    assert not three_relay_device.get_gpio("B2", refresh=True), (
        "channel 3's timer did not fire"
    )


# --- plain commands cancel the timer -------------------------------------


def test_plain_on_cancels_pending_timer(device: Device) -> None:
    """ON means "stay on" — the pending auto-OFF must be dropped."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    device.step_time(6_000)  # past the original 10 s deadline
    assert device.get_gpio("B0", refresh=True), (
        "auto-OFF fired after a plain ON — the timer was not cancelled"
    )


def test_plain_off_cancels_pending_timer(device: Device) -> None:
    """OFF turns the relay off; the stale timer must not fire later."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF)
    assert not device.get_gpio("B0", refresh=True)
    device.step_time(6_000)
    assert not device.get_gpio("B0", refresh=True)


def test_toggle_command_cancels_pending_timer(device: Device) -> None:
    """TOGGLE flips the relay and must cancel the pending auto-OFF."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE)
    # The relay was ON, so the toggle turned it OFF.
    assert not device.get_gpio("B0", refresh=True)
    device.step_time(6_000)
    assert not device.get_gpio("B0", refresh=True)


def test_off_with_effect_cancels_pending_timer(device: Device) -> None:
    """OFF with effect (0x40) is an OFF — it cancels the timer."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.call_zigbee_cmd(
        ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_OFF_WITH_EFFECT, payload_bytes=[0x00]
    )
    assert not device.get_gpio("B0", refresh=True)
    device.step_time(6_000)
    assert not device.get_gpio("B0", refresh=True)


def test_on_with_recall_global_scene_cancels_pending_timer(device: Device) -> None:
    """ON with recall global scene (0x41) is an ON — it cancels the timer."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_ON_OFF,
        ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE,
        payload_bytes=[0x00, 0x00, 0x00, 0x00],
    )
    assert device.get_gpio("B0", refresh=True)
    device.step_time(6_000)
    assert device.get_gpio("B0", refresh=True), (
        "auto-OFF fired after on-with-recall — the timer was not cancelled"
    )


def test_level_move_cancels_pending_timer(device: Device) -> None:
    """Level control with on/off (level 0) cancels the timer."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
        payload_bytes=[0x00, 0x00, 0x00],
    )
    assert not device.get_gpio("B0", refresh=True)
    device.step_time(6_000)
    assert not device.get_gpio("B0", refresh=True)


def test_attribute_write_does_not_cancel_pending_timer(device: Device) -> None:
    """Writing the onOff attribute is not a relay action — the timer stays."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.write_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF, 1)
    device.step_time(10_500)
    assert not device.get_gpio("B0", refresh=True), (
        "the timer did not fire — an attribute write must not cancel it"
    )


# --- a second On With Timed Off replaces the first ------------------------


def test_second_timed_off_replaces_the_pending_deadline(device: Device) -> None:
    """A second On With Timed Off must REPLACE the pending deadline, not
    stack on top of it and not cancel it.

    The replacement starts counting from the moment it arrives.
    """
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    # Replace with a 20 s timer.
    _on_with_timed_off(device, ep, on_time=200)
    # Past the FIRST deadline (10 s from the start) — still ON, because the
    # second command moved the deadline to 5 + 20 = 25 s.
    device.step_time(6_000)
    assert device.get_gpio("B0", refresh=True), (
        "the first timer fired — the second command did not replace it"
    )
    # Past the SECOND deadline (25 s from the start).
    device.step_time(15_000)
    assert not device.get_gpio("B0", refresh=True), (
        "the replacement timer did not fire at its own deadline"
    )


def test_second_timed_off_with_zero_turns_off_now(device: Device) -> None:
    """A replacement of (0,0) is a plain OFF and cancels the old timer."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    _on_with_timed_off(device, ep, on_time=0, off_wait_time=0)
    assert not device.get_gpio("B0", refresh=True)
    device.step_time(6_000)
    assert not device.get_gpio("B0", refresh=True)


def test_second_timed_off_keeps_relay_on(device: Device) -> None:
    """Replacing the timer must not toggle the relay off and on again —
    the relay stays ON across the replacement."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    _on_with_timed_off(device, ep, on_time=200)
    assert device.get_gpio("B0", refresh=True)
    assert device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) == "1"


# --- the physical button ---------------------------------------------------


def test_physical_button_cancels_pending_timer(device: Device) -> None:
    """A button press drives the relay, which cancels the pending auto-OFF.

    The ONOFF action is used deliberately: it drives the relay ON without
    changing its state, so the relay stays ON. If the timer had survived,
    it would switch the relay OFF at its deadline — which is exactly what
    this test watches for.
    """
    ep = 2
    _make_momentary(device, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF)
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    assert device.get_gpio("B0")
    device.step_time(5_000)
    device.click_button("A0")
    # The press drove the relay ON again — the state did not change.
    assert device.get_gpio("B0", refresh=True)
    # Past the original deadline. The button cancelled the timer, so the
    # relay must still be ON.
    device.step_time(6_000)
    assert device.get_gpio("B0", refresh=True), (
        "auto-OFF fired after a button press — the button did not cancel it"
    )


def test_physical_button_toggle_turns_the_relay_off(device: Device) -> None:
    """With the toggle action a button press flips the relay."""
    ep = 2
    _make_momentary(
        device, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE
    )
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.click_button("A0")
    assert not device.get_gpio("B0", refresh=True), (
        "the button did not toggle the relay"
    )


def test_physical_button_long_press_cancels_pending_timer(device: Device) -> None:
    """A long press also drives the relay and cancels the timer.

    RELAY_MODE_LONG drives the relay with a toggle on the long press
    itself. The relay therefore ends up OFF, which is indistinguishable
    from the auto-OFF having fired. The proof that the timer was
    cancelled is the report count: the timer firing would push a second
    onOff report (the relay was already off, so no third).
    """
    ep = 2
    _make_momentary(device, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF)
    device.write_zigbee_attr(
        1,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_LONG,
    )
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(5_000)
    device.long_click_button("A0", duration_ms=1_000)
    # The long press toggled the relay off.
    assert not device.get_gpio("B0", refresh=True)
    reports_after_press = _onoff_reports(device, ep)
    # Past the original deadline. If the timer had survived, it would
    # fire here and push another report.
    device.step_time(6_000)
    assert _onoff_reports(device, ep) == reports_after_press, (
        "the auto-OFF fired after a long press — the press did not cancel it"
    )
    assert not device.get_gpio("B0", refresh=True)


def test_button_in_detached_mode_leaves_the_timer_running(device: Device) -> None:
    """A detached switch does not drive the relay, so the timer survives."""
    ep = 2
    _make_momentary(device, 1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF)
    device.write_zigbee_attr(
        1,
        ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
        ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
        ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    )
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.click_button("A0")
    assert device.get_gpio("B0", refresh=True)
    device.step_time(10_500)
    assert not device.get_gpio("B0", refresh=True), (
        "the timer did not fire in detach mode"
    )


# --- cross-endpoint isolation --------------------------------------------


def test_button_of_the_other_endpoint_does_not_cancel_this_timer(
    two_relay_device: Device,
) -> None:
    """Switch 2 drives relay 2 (endpoint 4). It must not touch the timer
    that belongs to relay 1 (endpoint 3)."""
    ep_a, ep_b = 3, 4
    _make_momentary(
        two_relay_device, 2, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_ONOFF
    )
    _on_with_timed_off(two_relay_device, ep_a, on_time=100)  # 10 s
    two_relay_device.step_time(5_000)
    # Press the button of the second relay.
    two_relay_device.click_button("A1")
    # Relay A's timer must still fire at its own deadline.
    two_relay_device.step_time(6_000)
    assert not two_relay_device.get_gpio("B0", refresh=True), (
        "the button of the other endpoint cancelled this relay's timer"
    )


def test_plain_on_on_the_other_endpoint_does_not_cancel_this_timer(
    two_relay_device: Device,
) -> None:
    """An ON command to relay B must not cancel relay A's timer."""
    ep_a, ep_b = 3, 4
    _on_with_timed_off(two_relay_device, ep_a, on_time=100)  # 10 s
    two_relay_device.step_time(5_000)
    two_relay_device.call_zigbee_cmd(ep_b, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    two_relay_device.step_time(6_000)
    assert not two_relay_device.get_gpio("B0", refresh=True), (
        "relay A's timer did not fire — the other endpoint's command broke it"
    )


def test_timed_off_on_the_other_endpoint_does_not_cancel_this_timer(
    two_relay_device: Device,
) -> None:
    """Arming a timer on relay B must not disturb relay A's timer."""
    ep_a, ep_b = 3, 4
    _on_with_timed_off(two_relay_device, ep_a, on_time=100)  # 10 s
    two_relay_device.step_time(5_000)
    _on_with_timed_off(two_relay_device, ep_b, on_time=200)
    two_relay_device.step_time(6_000)
    assert not two_relay_device.get_gpio("B0", refresh=True), (
        "relay A's timer did not fire — the other endpoint's timer broke it"
    )


# --- the timer firing on its own -----------------------------------------


def test_timer_fires_and_leaves_the_relay_off(device: Device) -> None:
    """After the deadline the relay is OFF and stays OFF."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(10_500)
    assert not device.get_gpio("B0", refresh=True)
    assert device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) == "0"


def test_timer_can_be_rearmed_after_it_fired(device: Device) -> None:
    """After the timer fired, a new On With Timed Off works again."""
    ep = 2
    _on_with_timed_off(device, ep, on_time=100)  # 10 s
    device.step_time(10_500)
    assert not device.get_gpio("B0", refresh=True)
    # Re-arm with a fresh timer.
    _on_with_timed_off(device, ep, on_time=100)
    assert device.get_gpio("B0", refresh=True)
    device.step_time(10_500)
    assert not device.get_gpio("B0", refresh=True)
