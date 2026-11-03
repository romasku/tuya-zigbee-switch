"""Tests for active-low relay support via the 'i' inversion flag (RB0i config)."""
import pytest

from client import StubProc
from conftest import Device


@pytest.fixture
def relay_inverted_device():
    p = StubProc(device_config="Mfg;Model;SA0u;RB0i;").start()
    try:
        yield Device(p)
    finally:
        p.stop()


def test_active_low_relay_on_gpio_is_low(relay_inverted_device: Device):
    relay_inverted_device.zcl_relay_on(2)
    assert not relay_inverted_device.get_gpio("B0", refresh=True)


def test_active_low_relay_off_gpio_is_high(relay_inverted_device: Device):
    relay_inverted_device.zcl_relay_on(2)
    relay_inverted_device.zcl_relay_off(2)
    assert relay_inverted_device.get_gpio("B0", refresh=True)
