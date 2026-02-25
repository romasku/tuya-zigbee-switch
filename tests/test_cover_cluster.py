"""Tests for cover functionality (input and output clusters)."""
import pytest

from client import StubProc
from conftest import Device, MINIMUM_SWITCH_TIME_MS
from zcl_consts import (
    ZCL_WINDOW_COVERING_MOVING_STOPPED,
    ZCL_WINDOW_COVERING_MOVING_OPENING,
    ZCL_WINDOW_COVERING_MOVING_CLOSING,
)


@pytest.fixture
def cover_device() -> Device:
    p = StubProc(device_config="X;Y;CA0A1;").start()
    try:
        d = Device(p)
        # Advance time past the motor protection delay. Rather than adding extra logic to the firmware code
        # to allow immediate command execution, we handle this constraint in the test setup by advancing time here.
        d.step_time(MINIMUM_SWITCH_TIME_MS)
        yield d
    finally:
        p.stop()


def test_cover_open(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Act
    d.zcl_cover_open(endpoint)

    # Assert
    assert d.get_gpio("A0", refresh=True)
    assert not d.get_gpio("A1", refresh=True)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING


def test_cover_close(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Act
    d.zcl_cover_close(endpoint)

    # Assert
    assert not d.get_gpio("A0", refresh=True)
    assert d.get_gpio("A1", refresh=True)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING


def test_cover_open_reversed(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Arrange
    d.zcl_cover_motor_reversal_set(endpoint, 1)

    # Act
    d.zcl_cover_open(endpoint)

    # Assert
    assert not d.get_gpio("A0", refresh=True)
    assert d.get_gpio("A1", refresh=True)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING


def test_cover_close_reversed(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Arrange
    d.zcl_cover_motor_reversal_set(endpoint, 1)

    # Act
    d.zcl_cover_close(endpoint)

    # Assert
    assert d.get_gpio("A0", refresh=True)
    assert not d.get_gpio("A1", refresh=True)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING


def test_cover_stop(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Arrange: Start the cover and wait for the minimum switch time
    d.zcl_cover_open(endpoint)
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Act
    d.zcl_cover_stop(endpoint)

    # Assert
    assert not d.get_gpio("A0", refresh=True)
    assert not d.get_gpio("A1", refresh=True)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED


def test_cover_immediate_restart(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Arrange: Start and stop the cover
    d.zcl_cover_open(endpoint)
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_stop(endpoint)

    # Act: Restart the cover immediately
    d.zcl_cover_open(endpoint)

    # Assert: The cover should restart only after the minimum switch time
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING


def test_cover_immediate_reversal(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Arrange: Start the cover moving
    d.zcl_cover_open(endpoint)

    # Act: Reverse the direction immediately
    d.zcl_cover_close(endpoint)

    # Assert: The cover should stop and reverse only after the minimum switch times
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING
