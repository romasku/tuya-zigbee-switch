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


# ============================================================================
# Position tracking tests
# ============================================================================

# Default travel time is 30000 ms.
# Default initial position is 50%.
TRAVEL_TIME_MS = 30_000
OVERRUN_DURATION_MS = 3_000
MANUAL_MODE_TIMEOUT_MS = 300_000


def test_cover_goto_position(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Arrange: First fully open the cover
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 100
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Act: Go to 50%
    d.zcl_cover_goto_position(endpoint, 50)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING

    # Wait for it to arrive
    d.step_time(TRAVEL_TIME_MS)

    # Assert: Cover is at 50% and stopped
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 50


def test_cover_position_with_slack(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Arrange: Set closed slack to 10% of total travel — a 10% dead zone near closed.
    d.zcl_cover_set_closed_deadzone(endpoint, 10)

    # Move to fully closed first
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 0
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Act: Open cover and advance ~5% of travel (within the 10% slack zone)
    d.zcl_cover_open(endpoint)
    # 10% slack = 3000ms with default open_time=300 (30s). Moving 1500ms (5%) stays inside.
    d.step_time(1500)
    d.zcl_cover_stop(endpoint)

    # Assert: Position should still be 0 since we're within the closed slack zone
    assert d.zcl_cover_get_position(endpoint) == 0


def test_cover_auto_stop_at_target(cover_device: Device):
    d = cover_device
    endpoint = 1

    # Arrange: Fully open, then go to 25%
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 100
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Act: Go to 25% — should auto-stop after 75% of travel_time
    d.zcl_cover_goto_position(endpoint, 25)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING

    # Wait half the expected travel — should still be moving
    d.step_time(TRAVEL_TIME_MS // 4)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING

    # Wait until the full expected travel completes
    d.step_time(TRAVEL_TIME_MS)

    # Assert: Cover auto-stopped at target
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 25


def test_cover_sequential_goto_positions(cover_device: Device):
    """Reproduce the bug: second goto_position command is ignored after first completes."""
    d = cover_device
    endpoint = 1

    # Arrange: Fully open the cover first
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 100
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # First goto: 50% — should work
    d.zcl_cover_goto_position(endpoint, 50)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 50
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Second goto: 25% — must also start the motor and reach target
    d.zcl_cover_goto_position(endpoint, 25)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING, \
        "Second goto_position was ignored — motor did not start"
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 25

    # Third goto: back to 75% (reversal direction)
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_goto_position(endpoint, 75)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING, \
        "Third goto_position was ignored — motor did not start"
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 75


def test_cover_goto_position_target_change_while_moving(cover_device: Device):
    """
    Reproduce the bug: changing the position target while already moving in the same direction.

    When cover_goto_position is called while the motor is moving and the new target is in the
    SAME direction as the current movement, cover_dispatch_action detects a 'duplicate' and
    returns early.  target_motor_position is updated but the stop_task is NOT rescheduled.
    The auto-stop fires at the OLD target time, then cover_auto_stop_handler snaps
    motor_position to the NEW (un-reached) target.

    Concrete scenario: cover is closing from 100% toward 50%.  Before that stop fires, we
    redirect to 25% (still closing, further away).  The fixed firmware must reschedule the
    stop for the longer travel and reach 25%.  The buggy firmware stops at the 50%-time and
    then snaps to 25%, so a subsequent goto_position(25) sees motor_position == target and
    silently does nothing.
    """
    d = cover_device
    endpoint = 1

    # Arrange: fully open
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 100
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Start closing toward 50% (duration = 15 000 ms)
    d.zcl_cover_goto_position(endpoint, 50)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING

    # Immediately redirect to 25% (same direction, but further away → 22 500 ms total)
    d.zcl_cover_goto_position(endpoint, 25)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING

    # After the original 50% stop time (15 000 ms) the cover must STILL be moving
    # (it has 7 500 ms left to reach 25%).  The bug causes it to stop here instead.
    d.step_time(15000 + 1000)  # a bit past the original 50% stop
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING, \
        "Motor stopped too early — stop_task was not rescheduled for the new target"

    # Let the remaining travel complete
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 25


# ============================================================================
# Bug regression tests
# ============================================================================


def test_cover_has_pending_movement_cleared_after_delay(cover_device: Device):
    """
    Regression: has_pending_movement was never cleared after cover_delay_handler fired.

    Scenario: open → stop immediately (triggers delayed stop) → wait for delay →
    re-open → stop immediately again.  If has_pending_movement is stale from the first
    delay, the second stop's duplicate-detection path will try to cancel an already-fired
    delay_task instead of scheduling a new delayed stop, leaving the motor running.
    """
    d = cover_device
    endpoint = 1

    # 1. Open the cover
    d.zcl_cover_open(endpoint)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # 2. Stop immediately — within RELAY_MIN_SWITCH_TIME_MS, so stop is delayed
    d.zcl_cover_stop(endpoint)
    # Motor should NOT have stopped yet (delay is pending)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # 3. Let the delay fire — motor should now be stopped
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED

    # 4. Open again — must wait for motor protection delay since the stop just happened
    d.zcl_cover_open(endpoint)
    # Motor protection delay is active, so open is pending
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # 5. Stop immediately again — same pattern as step 2
    d.zcl_cover_stop(endpoint)
    # Motor should NOT have stopped yet (delay is pending)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED, \
        "Motor did not stop — has_pending_movement was not cleared after first delay"


def test_cover_rapid_direction_changes(cover_device: Device):
    """
    Regression: cover_defer_action did not unschedule the existing delay_task
    before scheduling a new one, causing double task execution.

    Rapid command sequence: open → close → open (all within RELAY_MIN_SWITCH_TIME_MS).
    The final state should reflect the last command (opening).
    """
    d = cover_device
    endpoint = 1

    # Open
    d.zcl_cover_open(endpoint)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Close immediately — triggers reversal (stop + delayed close)
    d.zcl_cover_close(endpoint)

    # Open immediately — should override the pending close
    d.zcl_cover_open(endpoint)

    # Let all delays resolve
    d.step_time(MINIMUM_SWITCH_TIME_MS * 3)

    # The cover should ultimately be opening (last command wins)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING, \
        "Last command did not win after rapid direction changes"


def test_cover_goto_position_after_open_stop_cycle(cover_device: Device):
    """
    Reproduce the user's real-world bug: open → stop → goto_position from Z2M.

    The user observed that after open+stop, a goto_position command caused the
    relay to actuate for only a very short time (matching the duration of the
    previous manual open/stop), then the position snapped back.
    """
    d = cover_device
    endpoint = 1

    # Open briefly, then stop
    d.zcl_cover_open(endpoint)
    d.step_time(2000)  # open for 2 seconds
    d.zcl_cover_stop(endpoint)
    d.step_time(MINIMUM_SWITCH_TIME_MS)  # wait for stop to take effect

    pos_after_stop = d.zcl_cover_get_position(endpoint)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED

    # Now send goto_position(80) — like Z2M would
    d.zcl_cover_goto_position(endpoint, 80)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING, \
        "goto_position did not start motor after open+stop cycle"

    # Motor should still be running after a short time (not immediately stopping)
    d.step_time(1000)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING, \
        "Motor stopped too early after goto_position — possible stale timing"

    # Let it complete
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 80


# ============================================================================
# Asymmetric open_time / close_time tests
# ============================================================================

def test_cover_asymmetric_open_faster(cover_device: Device):
    """open_time=150 (15 s) is half the close_time=300 (30 s).
    Starting from 0% (fully closed), opening to 100% takes 15 000 ms travel
    + OVERRUN_DURATION_MS overrun = 18 000 ms total."""
    d = cover_device
    endpoint = 1

    d.zcl_cover_set_open_time(endpoint, 150)
    d.zcl_cover_set_close_time(endpoint, 300)

    # Move to fully closed first
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 0
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Open — 15 000 ms travel to 100% + 3 000 ms overrun = 18 000 ms total
    d.zcl_cover_open(endpoint)
    d.step_time(15000 + OVERRUN_DURATION_MS)

    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 100


def test_cover_asymmetric_close_faster(cover_device: Device):
    """close_time=150 (15 s) is half the open_time=300 (30 s).
    Starting from 100% (fully open), closing to 0% takes 15 000 ms travel
    + OVERRUN_DURATION_MS overrun = 18 000 ms total."""
    d = cover_device
    endpoint = 1

    d.zcl_cover_set_open_time(endpoint, 300)
    d.zcl_cover_set_close_time(endpoint, 150)

    # Move to fully open first
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 100
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Close — 15 000 ms travel to 0% + 3 000 ms overrun = 18 000 ms total
    d.zcl_cover_close(endpoint)
    d.step_time(15000 + OVERRUN_DURATION_MS)

    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 0


def test_cover_asymmetric_goto_position_open(cover_device: Device):
    """With open_time=150 (15 000 ms full travel), goto_position(50) from 0%
    should auto-stop at 50% after exactly 7 500 ms."""
    d = cover_device
    endpoint = 1

    d.zcl_cover_set_open_time(endpoint, 150)
    d.zcl_cover_set_close_time(endpoint, 300)

    # Start from fully closed
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 0
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    d.zcl_cover_goto_position(endpoint, 50)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Still moving just before the expected stop time
    d.step_time(7000)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Let the remaining 500 ms elapse
    d.step_time(1000)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 50


def test_cover_asymmetric_goto_position_close(cover_device: Device):
    """With close_time=150 (15 000 ms full travel), goto_position(50) from 100%
    should auto-stop at 50% after exactly 7 500 ms."""
    d = cover_device
    endpoint = 1

    d.zcl_cover_set_open_time(endpoint, 300)
    d.zcl_cover_set_close_time(endpoint, 150)

    # Start from fully open
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 100
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    d.zcl_cover_goto_position(endpoint, 50)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING

    # Still moving just before the expected stop time
    d.step_time(7000)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING

    # Let the remaining 500 ms elapse
    d.step_time(1000)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 50


def test_cover_asymmetric_position_tracking_mid_stop(cover_device: Device):
    """open_time=200 (20 000 ms), close_time=400 (40 000 ms).
    Opening for 10 000 ms (50% of open travel) should land at 50%.
    Then closing for 20 000 ms (50% of close travel) should return to 0%."""
    d = cover_device
    endpoint = 1

    d.zcl_cover_set_open_time(endpoint, 200)
    d.zcl_cover_set_close_time(endpoint, 400)

    # Start from fully closed
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 0
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Open for 10 000 ms — half of 20 000 ms → 50%
    d.zcl_cover_open(endpoint)
    d.step_time(10000)
    d.zcl_cover_stop(endpoint)
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 50

    # Close for 20 000 ms — half of 40 000 ms → back to 0%
    d.zcl_cover_close(endpoint)
    d.step_time(20000)
    d.zcl_cover_stop(endpoint)
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 0


def test_cover_asymmetric_slack(cover_device: Device):
    """closed_deadzone is a percentage of physical travel, independent of directional speed.
    With closed_deadzone=10 (10%), the dead zone is 1 000 basis points.
    Opening at open_time=150 (15 000 ms full travel), 1 500 ms covers
    1 500/15 000 * 10 000 = 1 000 bp — exactly at the edge of the slack zone
    (motor_position <= closed_deadzone_bp is True) → position stays at 0%.
    With the old time-based formula this would have required different values for
    each direction; now one percentage works regardless of speed asymmetry."""
    d = cover_device
    endpoint = 1

    d.zcl_cover_set_open_time(endpoint, 150)
    d.zcl_cover_set_close_time(endpoint, 300)
    d.zcl_cover_set_closed_deadzone(endpoint, 10)  # 10% = 1000 bp, independent of direction

    # Start from fully closed
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 0
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Open for 1 500 ms — within the slack zone
    d.zcl_cover_open(endpoint)
    d.step_time(1500)
    d.zcl_cover_stop(endpoint)
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    assert d.zcl_cover_get_position(endpoint) == 0, \
        "Position should remain 0 while inside the closed slack zone"


def test_cover_close_time_fallback_to_open_time(cover_device: Device):
    """close_time=0 (the default sentinel) means fall back to open_time.
    With open_time=150 and close_time=0, closing takes the same 15 000 ms travel
    as opening, plus OVERRUN_DURATION_MS overrun at the end position."""
    d = cover_device
    endpoint = 1

    d.zcl_cover_set_open_time(endpoint, 150)
    # close_time left at 0 (default — falls back to open_time)

    # Move to fully open first (from default 50%: 7 500 ms travel + 3 000 ms overrun = 10 500 ms)
    d.zcl_cover_open(endpoint)
    d.step_time(15000)
    assert d.zcl_cover_get_position(endpoint) == 100
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Close — 15 000 ms travel to 0% + 3 000 ms overrun = 18 000 ms total
    d.zcl_cover_close(endpoint)
    d.step_time(15000 + OVERRUN_DURATION_MS)

    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED, \
        "Motor did not stop — close_time fallback to open_time did not work"
    assert d.zcl_cover_get_position(endpoint) == 0


def test_cover_both_times_default(cover_device: Device):
    """When neither open_time nor close_time is explicitly set (open_time=300,
    close_time=0), the built-in default of 30 seconds applies to both directions.
    A full 0% → 100% open takes TRAVEL_TIME_MS + OVERRUN_DURATION_MS total."""
    d = cover_device
    endpoint = 1

    # Start from fully closed (from 50%: 15 000 ms travel + 3 000 ms overrun = 18 000 ms)
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS)
    assert d.zcl_cover_get_position(endpoint) == 0
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Open from 0% — 30 000 ms travel + 3 000 ms overrun = 33 000 ms total
    d.zcl_cover_open(endpoint)
    d.step_time(TRAVEL_TIME_MS + OVERRUN_DURATION_MS)

    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 100


# ============================================================================
# Overrun tests
# ============================================================================

def test_cover_overrun_on_open_from_partial(cover_device: Device):
    """Opening from 50% to 100% runs the motor for travel_time + OVERRUN_DURATION_MS.
    The position is clamped to 100% as soon as the tracker reaches the end, but the motor
    continues for OVERRUN_DURATION_MS to re-align drift accumulated from sub-range usage."""
    d = cover_device
    endpoint = 1

    # Start from the default 50%, open toward 100%.
    # With open_time=300 (30 000 ms full travel): 15 000 ms travel + 3 000 ms overrun = 18 000 ms total.
    d.zcl_cover_open(endpoint)

    # At travel completion the position is already clamped to 100%, but the motor is still running.
    d.step_time(TRAVEL_TIME_MS // 2)  # 15 000 ms
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING
    assert d.zcl_cover_get_position(endpoint) == 100

    # Motor stops after the additional overrun period.
    d.step_time(OVERRUN_DURATION_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 100


def test_cover_overrun_on_close_from_partial(cover_device: Device):
    """Closing from 50% to 0% runs the motor for travel_time + OVERRUN_DURATION_MS.
    The position is clamped to 0% as soon as the tracker reaches the end, but the motor
    continues for OVERRUN_DURATION_MS to re-align drift accumulated from sub-range usage."""
    d = cover_device
    endpoint = 1

    # Start from the default 50%, close toward 0%.
    # With open_time=300 (30 000 ms full travel): 15 000 ms travel + 3 000 ms overrun = 18 000 ms total.
    d.zcl_cover_close(endpoint)

    # At travel completion the position is already clamped to 0%, but the motor is still running.
    d.step_time(TRAVEL_TIME_MS // 2)  # 15 000 ms
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING
    assert d.zcl_cover_get_position(endpoint) == 0

    # Motor stops after the additional overrun period.
    d.step_time(OVERRUN_DURATION_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 0


def test_cover_overrun_total_duration_open_full(cover_device: Device):
    """A full 0% \u2192 100% travel takes exactly TRAVEL_TIME_MS + OVERRUN_DURATION_MS.
    The motor is still running just before the total duration ends and stops exactly
    at the boundary, confirming the overrun is always appended after the travel time."""
    d = cover_device
    endpoint = 1

    # Move to fully closed first (from 50%: 15 000 ms travel + 3 000 ms overrun = 18 000 ms)
    d.zcl_cover_close(endpoint)
    d.step_time(TRAVEL_TIME_MS // 2 + OVERRUN_DURATION_MS)
    assert d.zcl_cover_get_position(endpoint) == 0
    d.step_time(MINIMUM_SWITCH_TIME_MS)

    # Open from 0% \u2192 100%: 30 000 ms travel + 3 000 ms overrun = 33 000 ms total
    d.zcl_cover_open(endpoint)

    # Still opening just before the total duration ends
    d.step_time(TRAVEL_TIME_MS + OVERRUN_DURATION_MS - 100)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Stopped after the full duration
    d.step_time(200)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 100


# ============================================================================
# Manual mode tests
# Manual mode is active when both open_time and close_time are zero.
# close_time defaults to 0, so setting open_time=0 is all that is needed.
# ============================================================================


def test_cover_manual_mode_open_and_close(cover_device: Device):
    """In manual mode the position attribute snaps immediately to 100% on open
    and to 0% on close, without waiting for the travel time to elapse."""
    d = cover_device
    endpoint = 1

    # Enter manual mode: close_time is already 0, only need to zero open_time.
    d.zcl_cover_set_open_time(endpoint, 0)

    # Open — position must snap to 100% immediately.
    d.zcl_cover_open(endpoint)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING
    assert d.zcl_cover_get_position(endpoint) == 100

    # Reverse to close. Motor-protection reversal: stop immediately, then close after
    # MINIMUM_SWITCH_TIME_MS. Both steps happen through the normal reversal path.
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    d.zcl_cover_close(endpoint)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED

    # After the protection delay the motor is closing and position snaps to 0%.
    d.step_time(MINIMUM_SWITCH_TIME_MS)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING
    assert d.zcl_cover_get_position(endpoint) == 0


def test_cover_manual_mode_safety_timeout(cover_device: Device):
    """In manual mode the motor auto-stops after MANUAL_MODE_TIMEOUT_MS (5 minutes)
    instead of the normal travel-time-based stop, ensuring the relay is always
    de-energized even if the user walks away."""
    d = cover_device
    endpoint = 1

    d.zcl_cover_set_open_time(endpoint, 0)

    d.zcl_cover_open(endpoint)
    assert d.zcl_cover_get_position(endpoint) == 100
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Still running just before the safety timeout.
    d.step_time(MANUAL_MODE_TIMEOUT_MS - 100)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

    # Auto-stopped after the full safety timeout.
    d.step_time(200)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 100


def test_cover_manual_mode_goto_position_ignored(cover_device: Device):
    """In manual mode, GoToLiftPercentage commands are silently ignored so the
    controller cannot auto-stop the motor before it reaches the physical end stop."""
    d = cover_device
    endpoint = 1

    # Enter manual mode. Default position is 50%.
    d.zcl_cover_set_open_time(endpoint, 0)

    d.zcl_cover_goto_position(endpoint, 80)
    assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
    assert d.zcl_cover_get_position(endpoint) == 50

