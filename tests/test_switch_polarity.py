"""Tests for switch polarity: pressed_when_high derived from pull resistor type."""
import pytest

from client import StubProc
from conftest import Device, DEBOUNCE_MS


@pytest.fixture
def pullup_device():
    p = StubProc(device_config="Mfg;Model;SA0u;RB0;").start()
    try:
        yield Device(p)
    finally:
        p.stop()


@pytest.fixture
def pulldown_device():
    p = StubProc(device_config="Mfg;Model;SA0d;RB0;").start()
    try:
        yield Device(p)
    finally:
        p.stop()


def test_pull_up_switch_presses_on_low(pullup_device: Device):
    """SA0u (pull-up): relay ON when GPIO goes LOW (active-low press)."""
    d = pullup_device
    d.set_gpio("A0", 0)  # press
    d.step_time(DEBOUNCE_MS + 10)
    assert d.get_gpio("B0", refresh=True)


def test_pull_down_switch_presses_on_high(pulldown_device: Device):
    """SA0d (pull-down): relay ON when GPIO goes HIGH (active-high press)."""
    d = pulldown_device
    d.set_gpio("A0", 1)  # press
    d.step_time(DEBOUNCE_MS + 10)
    assert d.get_gpio("B0", refresh=True)
