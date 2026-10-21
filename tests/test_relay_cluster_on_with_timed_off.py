"""Tests for the OnOff cluster "On With Timed Off" command (ZCL 0x42).

The stub REPL runs with time frozen by default; we use `step_time` to advance
the virtual clock so scheduled timers can fire deterministically. The stub's
`step_time` command also calls `app_task()` once after stepping, so the relay
cluster polling tick re-evaluates the deadline against the new time.
"""

import pytest

from tests.client import StubProc
from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
    ZCL_CMD_OFF_WITH_EFFECT,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ONOFF_TOGGLE,
    ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE,
    ZCL_CMD_ON_WITH_TIMED_OFF,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
)


# The 5-byte onWithTimedOff payload: [onOffCtrl, onTime_LO, onTime_HI,
# offWaitTime_LO, offWaitTime_HI]. The Telink SDK stores onTime as
# (pData[2]<<8)|pData[1] (LE), so byte[1] is the LO byte. The romasku
# code reads the bytes directly to avoid struct-cast layout drift
# between the x86_64 stub (where the bitfield union is 4 bytes) and
# tc32 (where it is 1 byte).
def _payload(on_time: int, off_wait_time: int) -> list[int]:
    on_lo, on_hi = on_time & 0xFF, (on_time >> 8) & 0xFF
    off_lo, off_hi = off_wait_time & 0xFF, (off_wait_time >> 8) & 0xFF
    return [0x00, on_lo, on_hi, off_lo, off_hi]


@pytest.fixture()
def timed_off_device() -> Device:
    # One button + one relay: switch_endpoint=1, relay_endpoint=2.
    cfg = "X;Y;SA0u;RB0;"
    with StubProc(device_config=cfg) as proc:
        yield Device(proc)


@pytest.fixture()
def multi_relay_timed_off_device() -> Device:
    # Two buttons + two relays: switch_eps=1,2; relay_eps=3,4.
    cfg = "X;Y;SA0u;SA1u;RB0;RB1;"
    with StubProc(device_config=cfg) as proc:
        yield Device(proc)


def _on_with_timed_off(
    device: Device, ep: int, on_time: int, off_wait_time: int
) -> None:
    device.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_ON_OFF,
        ZCL_CMD_ON_WITH_TIMED_OFF,
        payload_bytes=_payload(on_time, off_wait_time),
    )


# --- plain ON/OFF/level — basic path, must not be broken by the timer ---

def test_plain_on_turns_relay_on(timed_off_device: Device) -> None:
    ep = 2
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    assert (
        timed_off_device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF)
        == "1"
    )
    assert timed_off_device.get_gpio("B0")


def test_plain_off_turns_relay_off(timed_off_device: Device) -> None:
    ep = 2
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    assert timed_off_device.get_gpio("B0")
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF)
    assert not timed_off_device.get_gpio("B0")
    assert (
        timed_off_device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF)
        == "0"
    )


def test_plain_on_cancels_pending_timer(timed_off_device: Device) -> None:
    """A fresh ON while a previous auto-OFF timer is pending must cancel
    that timer — the user expects "ON" to mean "stay on"."""
    ep = 2
    _on_with_timed_off(timed_off_device, ep, on_time=10, off_wait_time=0)
    assert timed_off_device.get_gpio("B0")
    # Step partway through the 1s timer.
    timed_off_device.step_time(500)
    # Fresh ON cancels the pending auto-OFF.
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    # Step past the original deadline.
    timed_off_device.step_time(800)
    assert timed_off_device.get_gpio("B0", refresh=True), (
        "Pending auto-OFF fired despite a fresh ON — cancel did not work"
    )


# --- onWithTimedOff happy path ---

def test_on_with_timed_off_turns_relay_on_immediately(
    timed_off_device: Device,
) -> None:
    ep = 2
    _on_with_timed_off(timed_off_device, ep, on_time=10, off_wait_time=0)
    assert (
        timed_off_device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF)
        == "1"
    )
    assert timed_off_device.get_gpio("B0")


def test_on_with_timed_off_turns_off_after_on_time(
    timed_off_device: Device,
) -> None:
    ep = 2
    _on_with_timed_off(timed_off_device, ep, on_time=10, off_wait_time=0)
    # on_time = 10 -> 1.0s. Step past the boundary.
    timed_off_device.step_time(1100)
    assert (
        timed_off_device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF)
        == "0"
    )
    assert not timed_off_device.get_gpio("B0")


def test_on_with_timed_off_does_not_fire_before_on_time(
    timed_off_device: Device,
) -> None:
    ep = 2
    _on_with_timed_off(timed_off_device, ep, on_time=20, off_wait_time=0)
    timed_off_device.step_time(500)  # half of 2.0s
    assert (
        timed_off_device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF)
        == "1"
    )
    assert timed_off_device.get_gpio("B0")


def test_on_with_timed_off_uses_off_wait_time_when_provided(
    timed_off_device: Device,
) -> None:
    """ZCL spec: if offWaitTime>0, the auto-OFF fires after offWaitTime,
    not onTime."""
    ep = 2
    # onTime=10 (1.0s), offWaitTime=30 (3.0s) — auto-OFF must fire at 3.0s.
    _on_with_timed_off(timed_off_device, ep, on_time=10, off_wait_time=30)
    timed_off_device.step_time(2000)  # past 1.0s, before 3.0s
    assert timed_off_device.get_gpio("B0")
    timed_off_device.step_time(1500)  # total 3.5s
    assert not timed_off_device.get_gpio("B0", refresh=True)


def test_on_with_timed_off_zero_both_turns_off(
    timed_off_device: Device,
) -> None:
    """ZCL spec: (onTime=0, offWaitTime=0) is a plain OFF."""
    ep = 2
    # Start ON
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    assert timed_off_device.get_gpio("B0")
    # (0,0) -> OFF now
    _on_with_timed_off(timed_off_device, ep, on_time=0, off_wait_time=0)
    assert not timed_off_device.get_gpio("B0", refresh=True)


def test_on_with_timed_off_only_off_wait_keeps_off(
    timed_off_device: Device,
) -> None:
    """(onTime=0, offWaitTime>0) — the romasku firmware implements this
    as a plain OFF (the "wait then ON" half of the ZCL spec is not
    implemented; Z2M does not exercise this case)."""
    ep = 2
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    assert timed_off_device.get_gpio("B0")
    _on_with_timed_off(timed_off_device, ep, on_time=0, off_wait_time=20)
    assert not timed_off_device.get_gpio("B0", refresh=True)
    # Stay off past the offWaitTime.
    timed_off_device.step_time(2_500)
    assert not timed_off_device.get_gpio("B0", refresh=True)


def test_on_with_timed_off_new_command_cancels_pending(
    timed_off_device: Device,
) -> None:
    """A second onWithTimedOff replaces the first. With onTime=0 in
    the replacement it becomes a plain OFF."""
    ep = 2
    _on_with_timed_off(timed_off_device, ep, on_time=50, off_wait_time=0)
    # Replacement with (0,0) — turns OFF now, cancels the 5s timer.
    _on_with_timed_off(timed_off_device, ep, on_time=0, off_wait_time=0)
    assert not timed_off_device.get_gpio("B0", refresh=True)
    # Past the original deadline.
    timed_off_device.step_time(5_500)
    assert not timed_off_device.get_gpio("B0", refresh=True), (
        "Old timer fired after (0,0) replacement — the replacement did "
        "not cancel the previous arming"
    )


def test_off_command_cancels_pending_timer(timed_off_device: Device) -> None:
    """A manual OFF cancels any pending auto-OFF."""
    ep = 2
    _on_with_timed_off(timed_off_device, ep, on_time=20, off_wait_time=0)
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF)
    # Past the original deadline.
    timed_off_device.step_time(2_500)
    assert not timed_off_device.get_gpio("B0", refresh=True)
    # And another auto-OFF must be armable fresh.
    _on_with_timed_off(timed_off_device, ep, on_time=10, off_wait_time=0)
    timed_off_device.step_time(1_200)
    assert not timed_off_device.get_gpio("B0", refresh=True)


def test_level_move_with_on_off_cancels_pending_timer(
    timed_off_device: Device,
) -> None:
    ep = 2
    _on_with_timed_off(timed_off_device, ep, on_time=50, off_wait_time=0)
    # level=0 -> off, which should cancel the pending timer.
    timed_off_device.call_zigbee_cmd(
        ep,
        ZCL_CLUSTER_LEVEL_CONTROL,
        ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
        payload_bytes=[0x00, 0x00, 0x00],  # level=0, trans_time=0
    )
    assert not timed_off_device.get_gpio("B0", refresh=True)
    timed_off_device.step_time(5_500)
    assert not timed_off_device.get_gpio("B0", refresh=True), (
        "Auto-OFF fired after a manual level=0 — the level handler did "
        "not cancel the pending timer"
    )


# --- regressions for the captured ZCL bugs ---

def test_on_with_timed_off_fires_after_hal_millis_wraparound(
    timed_off_device: Device,
) -> None:
    """Regression: the deadline must survive the 32-bit hal_millis()
    wraparound (the Telink 8258 16MHz hardware counter wraps every
    ~268s; the stub mimics this with a 400s wrap).

    The romasku code now uses a 64-bit deadline (`hal_millis_64() +
    delay_ms`) and a 64-bit comparison, so even if a 32-bit
    `hal_millis()` cast would have shown the deadline "in the past",
    the 64-bit timeline is monotonic and the timer fires at the right
    time.
    """
    ep = 2
    # Push the clock to 395s (just under the 400s wrap). hal_millis()=395_000,
    # hal_millis_64()=395_000.
    timed_off_device.step_time(395_000)
    # 1s on-time. 64-bit deadline = 396_000.
    _on_with_timed_off(timed_off_device, ep, on_time=10, off_wait_time=0)
    # Step 10s. hal_millis_64()=405_000, hal_millis() mod 400_000 = 5_000.
    timed_off_device.step_time(10_000)
    # The timer must fire: 9s past the 1s deadline.
    assert (
        timed_off_device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF)
        == "0"
    ), "relay should be OFF after deadline even though 32-bit hal_millis() wrapped"
    assert not timed_off_device.get_gpio("B0")


def test_malformed_payload_returns_malformed_command(
    timed_off_device: Device,
) -> None:
    ep = 2
    # Short payload (< 5 bytes) -> MALFORMED, not PROCESSED.
    res = timed_off_device.call_zigbee_cmd_raw(
        ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ON_WITH_TIMED_OFF, payload=bytes([0x01])
    )
    assert res.get("result") == "MALFORMED_COMMAND", res


def test_on_with_timed_off_uses_le_wire_layout(
    timed_off_device: Device,
) -> None:
    """Regression: onTime / offWaitTime are read as little-endian u16s
    from cmd_payload[1..2] and [3..4]. Telink SDK stores onTime as
    `(pData[2]<<8) | pData[1]`, so the LO byte is at cmd_payload[1].

    If a future change flips to big-endian (or uses a struct cast that
    gives a different layout on x86_64 vs tc32), this test sees the
    auto-OFF fire at the wrong time and fails.
    """
    ep = 2
    # LE 0x1234 onTime (= 4660 in 0.1s = 466.0s), 0 offWaitTime.
    payload = [0x00, 0x34, 0x12, 0x00, 0x00]
    timed_off_device.call_zigbee_cmd(
        ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ON_WITH_TIMED_OFF,
        payload_bytes=payload,
    )
    assert timed_off_device.get_gpio("B0")
    # 460s into a 466s timer — must still be ON.
    timed_off_device.step_time(460_000)
    assert timed_off_device.get_gpio("B0", refresh=True), (
        "Auto-OFF fired too early — likely a byte-order bug, the "
        "romasku code is reading onTime in the wrong order"
    )
    # 7s later (467s total) — must be OFF.
    timed_off_device.step_time(7_000)
    assert not timed_off_device.get_gpio("B0", refresh=True), (
        "Auto-OFF did not fire at the deadline"
    )


# --- multi-relay independence ---

def test_multi_relay_timers_are_independent(
    multi_relay_timed_off_device: Device,
) -> None:
    """A timer on relay A does not affect relay B."""
    ep_a, ep_b = 3, 4
    _on_with_timed_off(multi_relay_timed_off_device, ep_a, on_time=10, off_wait_time=0)
    _on_with_timed_off(multi_relay_timed_off_device, ep_b, on_time=40, off_wait_time=0)
    assert multi_relay_timed_off_device.get_gpio("B0")
    assert multi_relay_timed_off_device.get_gpio("B1")
    # Step past relay A's 1s deadline. A turns off, B stays on.
    multi_relay_timed_off_device.step_time(1_500)
    assert not multi_relay_timed_off_device.get_gpio("B0", refresh=True)
    assert multi_relay_timed_off_device.get_gpio("B1", refresh=True)
    # Step past relay B's 4s deadline. B turns off.
    multi_relay_timed_off_device.step_time(3_000)
    assert not multi_relay_timed_off_device.get_gpio("B1", refresh=True)


# --- detach mode (button does not affect the relay) ---

def test_on_with_timed_off_works_with_detached_switch() -> None:
    cfg = "X;Y;SA0u;RB0;"
    with StubProc(device_config=cfg) as proc:
        device = Device(proc)
        device.zcl_switch_relay_mode_set(
            1, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED
        )

        relay_ep = 2
        _on_with_timed_off(device, relay_ep, on_time=10, off_wait_time=0)
        assert device.get_gpio("B0")
        # A physical button press in detach mode must not affect the
        # relay or its timer.
        device.click_button("A0")
        assert device.get_gpio("B0")  # still ON
        # After 1s the timer must fire regardless of the button press.
        device.step_time(1100)
        assert not device.get_gpio("B0", refresh=True)


def test_physical_button_in_detach_mode_does_not_cancel_timer() -> None:
    """In detach mode the button does not change the relay, so its
    on_change callback does not fire — and the pending auto-OFF timer
    must still run."""
    cfg = "X;Y;SA0u;RB0;"
    with StubProc(device_config=cfg) as proc:
        device = Device(proc)
        device.zcl_switch_relay_mode_set(
            1, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED
        )

        relay_ep = 2
        _on_with_timed_off(device, relay_ep, on_time=10, off_wait_time=0)
        # Press the button — in detach mode this must NOT affect the relay.
        device.click_button("A0")
        assert device.get_gpio("B0")
        # And the timer must still fire at the deadline.
        device.step_time(1100)
        assert not device.get_gpio("B0", refresh=True)


# --- onWithTimedOff on a relay that is already ON ---

def test_on_with_timed_off_works_when_relay_already_on(
    timed_off_device: Device,
) -> None:
    """Per ZCL spec, onWithTimedOff is the canonical "schedule an
    auto-OFF on top of an already-ON relay" command — the controller
    doesn't have to send a separate On. Sending it to a relay that's
    already ON must schedule the auto-OFF without the relay on_change
    callback cancelling the freshly installed timer (relay_on() is a
    no-op on a relay that is already on, so on_change is not invoked).
    """
    ep = 2
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    assert timed_off_device.get_gpio("B0")

    # onTime=50 (5s). The handler must (a) not toggle the relay,
    # (b) install a fresh auto-OFF timer, (c) fire it at 5s.
    _on_with_timed_off(timed_off_device, ep, on_time=50, off_wait_time=0)

    # Relay stays ON right after the command.
    assert timed_off_device.get_gpio("B0", refresh=True)
    # And is still ON at the halfway point.
    timed_off_device.step_time(2_500)
    assert timed_off_device.get_gpio("B0", refresh=True)
    # And finally turns OFF at 5s.
    timed_off_device.step_time(2_600)
    assert not timed_off_device.get_gpio("B0", refresh=True)
