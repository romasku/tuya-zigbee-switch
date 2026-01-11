"""Tests for cover functionality (input and output clusters)."""
import pytest

from client import StubProc
from conftest import Device
from zcl_consts import (
    ZCL_WINDOW_COVERING_MOVING_STOPPED,
    ZCL_WINDOW_COVERING_MOVING_OPENING,
    ZCL_WINDOW_COVERING_MOVING_CLOSING,
)

MINIMUM_ON_TIME_MS = 200
MINIMUM_OFF_TIME_MS = 500

def test_cover_open_command():
    p = StubProc(device_config="X;Y;CA0A1;").start()
    try:
        d = Device(p)
        endpoint = 1

        # Open command should activate the first relay
        d.zcl_cover_open(endpoint)
        assert d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING
        d.step_time(MINIMUM_ON_TIME_MS)

        # Stop command should deactivate both relays
        d.zcl_cover_stop(endpoint)
        assert not d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
        d.step_time(MINIMUM_OFF_TIME_MS)

        # After motor reversal, open command should activate the second relay
        d.zcl_cover_motor_reversal_set(endpoint, 1)
        d.zcl_cover_open(endpoint)
        assert not d.get_gpio("A0", refresh=True)
        assert d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING
    finally:
        p.stop()


def test_cover_close_command():
    p = StubProc(device_config="X;Y;CA0A1;").start()
    try:
        d = Device(p)
        endpoint = 1

        # Close command should activate the second relay
        d.zcl_cover_close(endpoint)
        assert not d.get_gpio("A0", refresh=True)
        assert d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING
        d.step_time(MINIMUM_ON_TIME_MS)

        # Stop command should deactivate both relays
        d.zcl_cover_stop(endpoint)
        assert not d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED
        d.step_time(MINIMUM_OFF_TIME_MS)

        # After motor reversal, close command should activate the first relay
        d.zcl_cover_motor_reversal_set(endpoint, 1)
        d.zcl_cover_close(endpoint)
        assert d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING
    finally:
        p.stop()


def test_cover_direction_change():
    p = StubProc(device_config="X;Y;CA0A1;").start()
    try:
        d = Device(p)
        endpoint = 1

        # Open command should work immediately
        d.zcl_cover_open(endpoint)
        assert d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

        # Close command should not have any effect before minimum on-time
        d.zcl_cover_close(endpoint)
        assert d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

        # Relay should deactivate after minimum on-time
        d.step_time(MINIMUM_ON_TIME_MS)
        assert not d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED

        # Opposite relay should activate after minimum off-time
        d.step_time(MINIMUM_OFF_TIME_MS)
        assert not d.get_gpio("A0", refresh=True)
        assert d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_CLOSING
    finally:
        p.stop()


def test_cover_restart_after_stop():
    p = StubProc(device_config="X;Y;CA0A1;").start()
    try:
        d = Device(p)
        endpoint = 1

        # Open command should work immediately
        d.zcl_cover_open(endpoint)
        assert d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

        # Stop command should not have any effect before minimum on-time
        d.zcl_cover_stop(endpoint)
        assert d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING

        # Relay should deactivate after minimum on-time
        d.step_time(MINIMUM_ON_TIME_MS)
        assert not d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED

        # Open command should not have any effect before minimum off-time
        d.zcl_cover_open(endpoint)
        assert not d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_STOPPED

        # Relay should activate after minimum off-time
        d.step_time(MINIMUM_OFF_TIME_MS)
        assert d.get_gpio("A0", refresh=True)
        assert not d.get_gpio("A1", refresh=True)
        assert d.zcl_cover_get_moving(endpoint) == ZCL_WINDOW_COVERING_MOVING_OPENING
    finally:
        p.stop()
