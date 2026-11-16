"""Tests for active-low cover relay support via the 'i' inversion flag (CA0A1i config)."""
import pytest

from client import StubProc
from conftest import Device, MINIMUM_SWITCH_TIME_MS


@pytest.fixture
def cover_device_inverted():
    p = StubProc(device_config="X;Y;CA0A1i;").start()
    try:
        d = Device(p)
        d.step_time(MINIMUM_SWITCH_TIME_MS)
        yield d
    finally:
        p.stop()


def test_active_low_cover_open_drives_open_relay_low(cover_device_inverted: Device):
    cover_device_inverted.zcl_cover_open(1)
    assert not cover_device_inverted.get_gpio("A0", refresh=True)
    assert cover_device_inverted.get_gpio("A1", refresh=True)


def test_active_low_cover_close_drives_close_relay_low(cover_device_inverted: Device):
    cover_device_inverted.zcl_cover_close(1)
    assert cover_device_inverted.get_gpio("A0", refresh=True)
    assert not cover_device_inverted.get_gpio("A1", refresh=True)
