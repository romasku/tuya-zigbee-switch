"""Tests for cover functionality (input and output clusters)."""
import pytest

from client import StubProc
from conftest import Device, MINIMUM_SWITCH_TIME_MS
from zcl_consts import (
    ZCL_WINDOW_COVERING_MOVING_STOPPED,
    ZCL_WINDOW_COVERING_MOVING_OPENING,
    ZCL_WINDOW_COVERING_MOVING_CLOSING,
)

# Travel times for calibrated covers (easy math: 1% per 100ms)
TRAVEL_TIME_MS = 10000


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


@pytest.fixture
def calibrated_cover_device() -> Device:
    """Cover device with position tracking enabled (travel_time = 10000ms each direction)."""
    p = StubProc(device_config="X;Y;CA0A1;").start()
    try:
        d = Device(p)
        d.step_time(MINIMUM_SWITCH_TIME_MS)
        endpoint = 1
        # Enable position tracking
        d.zcl_cover_set_open_time(endpoint, TRAVEL_TIME_MS)
        d.zcl_cover_set_close_time(endpoint, TRAVEL_TIME_MS)
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


# Position tracking tests
def test_position_default_is_50(calibrated_cover_device: Device):
    """Position should default to 50 (unknown)."""
    d = calibrated_cover_device
    endpoint = 1
    assert d.zcl_cover_get_position(endpoint) == 50


def test_position_updates_after_full_open(calibrated_cover_device: Device):
    """After opening for full travel time from position 50, position should be 0 (fully open)."""
    d = calibrated_cover_device
    endpoint = 1

    # Act: Open for the full travel time, then stop
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    d.zcl_cover_stop(endpoint)

    # Assert: Position should be fully open (0)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 0


def test_position_updates_after_full_close(calibrated_cover_device: Device):
    """After closing for full travel time from position 0, position should be 100 (fully closed)."""
    d = calibrated_cover_device
    endpoint = 1

    # Arrange: First open fully to get position to 0
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    d.zcl_cover_stop(endpoint)
    assert d.zcl_cover_get_position(endpoint) == 0

    # Act: Close for the full travel time
    d.step_time(MINIMUM_SWITCH_TIME_MS)  # Motor protection delay
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    d.zcl_cover_stop(endpoint)

    # Assert: Position should be fully closed (100)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 100


def test_position_partial_open(calibrated_cover_device: Device):
    """Opening for 50% of travel time from position 100 should result in position 50."""
    d = calibrated_cover_device
    endpoint = 1

    # Arrange: First close fully to get position to 100
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    d.zcl_cover_stop(endpoint)
    assert d.zcl_cover_get_position(endpoint) == 100

    # Act: Open for 50% of travel time
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS // 2)
    d.zcl_cover_stop(endpoint)

    # Assert: Position should be ~50 (within 1 due to integer math)
    pos = d.zcl_cover_get_position(endpoint)
    assert 49 <= pos <= 51


def test_go_to_lift_percentage_from_closed(calibrated_cover_device: Device):
    """GoToLiftPercentage(30) from fully closed position should open to 30."""
    d = calibrated_cover_device
    endpoint = 1

    # Arrange: Close fully
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    d.zcl_cover_stop(endpoint)
    assert d.zcl_cover_get_position(endpoint) == 100

    # Act: Go to 30%
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_go_to_lift_percentage(endpoint, 30)
    expected_run_time = (100 - 30) * TRAVEL_TIME_MS // 100
    d.step_time(expected_run_time + MINIMUM_SWITCH_TIME_MS)

    # Assert: Position should be exactly 30
    pos = d.zcl_cover_get_position(endpoint)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert pos == 30


def test_go_to_lift_percentage_from_open(calibrated_cover_device: Device):
    """GoToLiftPercentage(70) from fully open position should close to 70."""
    d = calibrated_cover_device
    endpoint = 1

    # Arrange: Open fully (starting position is 50, so open first)
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    d.zcl_cover_stop(endpoint)
    assert d.zcl_cover_get_position(endpoint) == 0

    # Act: Go to 70%
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_go_to_lift_percentage(endpoint, 70)
    expected_run_time = 70 * TRAVEL_TIME_MS // 100
    d.step_time(expected_run_time + MINIMUM_SWITCH_TIME_MS)

    # Assert: Position should be exactly 70
    pos = d.zcl_cover_get_position(endpoint)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert pos == 70


def test_go_to_lift_percentage_already_there(calibrated_cover_device: Device):
    """GoToLiftPercentage to current position should stop immediately."""
    d = calibrated_cover_device
    endpoint = 1

    # Act: Go to 50 (already there)
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_go_to_lift_percentage(endpoint, 50)

    # Assert: Motor should stop immediately
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 50


def test_go_to_lift_percentage_stops_at_target(calibrated_cover_device: Device):
    """GoToLiftPercentage should stop the motor exactly when reaching target."""
    d = calibrated_cover_device
    endpoint = 1

    # Arrange: Start from fully open
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    d.zcl_cover_stop(endpoint)

    # Act: Go to 50%
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_go_to_lift_percentage(endpoint, 50)

    # Assert: Motor is moving
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING

    # Step to target time and verify it stops
    expected_run_time = 50 * TRAVEL_TIME_MS // 100
    d.step_time(expected_run_time + MINIMUM_SWITCH_TIME_MS)

    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    pos = d.zcl_cover_get_position(endpoint)
    assert pos == 50


def test_go_to_lift_percentage_cancelled_by_stop(calibrated_cover_device: Device):
    """Manual STOP should cancel GoToLiftPercentage and preserve position."""
    d = calibrated_cover_device
    endpoint = 1

    # Arrange: Start from fully open
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    d.zcl_cover_stop(endpoint)

    # Act: Go to 30%, but stop halfway through the run
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_go_to_lift_percentage(endpoint, 30)

    # run_ms = 30 * TRAVEL_TIME_MS / 100 = 3000ms; stop at the midpoint (1500ms)
    run_ms = 30 * TRAVEL_TIME_MS // 100
    d.step_time(run_ms // 2)
    d.zcl_cover_stop(endpoint)

    # Assert: Motor stopped halfway to target — position is 15 (0 + 15)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    pos = d.zcl_cover_get_position(endpoint)
    assert pos == 15


def test_go_to_lift_percentage_no_tracking(cover_device: Device):
    """GoToLiftPercentage should be ignored when position tracking is not configured."""
    d = cover_device
    endpoint = 1
    # This device has no travel times configured, so GoToLiftPercentage should have no effect

    # Act: Try to go to 70%
    d.zcl_cover_go_to_lift_percentage(endpoint, 70)
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Assert: Motor should not move
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
