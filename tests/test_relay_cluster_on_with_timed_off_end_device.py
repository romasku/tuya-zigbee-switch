"""Countdown timer tests for the end-device build.

The end-device firmware enables the same countdown timer as the router
build. In the stub the end-device binary differs only in the
poll-control cluster, so the timer expectations are the same.
"""

import pytest

from tests.client import StubProc
from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ON_WITH_TIMED_OFF,
)

END_DEVICE_CMD = ["./build/stub/stub_end_device"]


def _payload(on_time: int, off_wait_time: int) -> list[int]:
    on_lo, on_hi = on_time & 0xFF, (on_time >> 8) & 0xFF
    off_lo, off_hi = off_wait_time & 0xFF, (off_wait_time >> 8) & 0xFF
    return [0x00, on_lo, on_hi, off_lo, off_hi]


@pytest.fixture()
def timed_off_device() -> Device:
    # One button + one relay: switch_endpoint=1, relay_endpoint=2.
    cfg = "X;Y;SA0u;RB0;"
    with StubProc(cmd=END_DEVICE_CMD, device_config=cfg) as proc:
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


def test_plain_on_cancels_pending_timer(timed_off_device: Device) -> None:
    ep = 2
    _on_with_timed_off(timed_off_device, ep, on_time=10, off_wait_time=0)
    assert timed_off_device.get_gpio("B0")
    timed_off_device.step_time(500)
    timed_off_device.call_zigbee_cmd(ep, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
    timed_off_device.step_time(800)
    assert timed_off_device.get_gpio("B0", refresh=True), (
        "Pending auto-OFF fired despite a fresh ON — cancel did not work"
    )
