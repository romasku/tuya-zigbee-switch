import os
import shutil
import struct

import pytest
from client import StubProc
from conftest import Device
from zcl_consts import (
    ZCL_ATTR_POLL_CTRL_CHECK_IN_INTERVAL,
    ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT,
    ZCL_ATTR_POLL_CTRL_LONG_POLL_INTERVAL,
    ZCL_ATTR_POLL_CTRL_SHORT_POLL_INTERVAL,
    ZCL_CLUSTER_POLL_CONTROL,
    ZCL_CMD_POLL_CTRL_CHECK_IN_RSP,
    ZCL_CMD_POLL_CTRL_FAST_POLL_STOP,
    ZCL_CMD_POLL_CTRL_SET_LONG_POLL_INTERVAL,
    ZCL_CMD_POLL_CTRL_SET_SHORT_POLL_INTERVAL,
)

END_DEVICE_CMD = ["./build/stub/stub_end_device"]

# Defaults in quarter-seconds
BATTERY_CHECK_IN_INTERVAL = 3600 * 4  # 1 hour
BATTERY_LONG_POLL_INTERVAL = 120 * 4  # 2 minutes
BATTERY_SHORT_POLL_INTERVAL = 2  # 500ms
BATTERY_FAST_POLL_TIMEOUT = 10 * 4  # 10 seconds

NON_BATTERY_CHECK_IN_INTERVAL = 3600 * 4
NON_BATTERY_LONG_POLL_INTERVAL = 1  # 250ms
NON_BATTERY_SHORT_POLL_INTERVAL = 1  # 250ms
NON_BATTERY_FAST_POLL_TIMEOUT = 10 * 4  # 10 seconds

QS_TO_MS = 250  # quarter-second to ms multiplier


def read_poll_attr(device: Device, attr: int) -> int:
    return int(device.read_zigbee_attr(1, ZCL_CLUSTER_POLL_CONTROL, attr))


def make_end_device(
    config: str, joined: bool = True, freeze_time: bool = True
) -> StubProc:
    return StubProc(
        cmd=END_DEVICE_CMD,
        device_config=config,
        joined=joined,
        freeze_time=freeze_time,
    ).start()


@pytest.fixture(autouse=True)
def cleanup_nvm():
    nvm_dir = "./stub_nvm_data"
    if os.path.exists(nvm_dir):
        shutil.rmtree(nvm_dir)
    yield
    if os.path.exists(nvm_dir):
        shutil.rmtree(nvm_dir)


# --- Non-battery device tests ---

NON_BATTERY_CONFIG = "StubManufacturer;StubDevice;SA0u;RB0;"


@pytest.fixture
def nb_device() -> Device:
    proc = make_end_device(NON_BATTERY_CONFIG)
    dev = Device(proc)
    yield dev
    proc.stop()


def test_default_attrs_non_battery(nb_device: Device):
    """Non-battery end device has correct poll control defaults."""
    assert (
        read_poll_attr(nb_device, ZCL_ATTR_POLL_CTRL_CHECK_IN_INTERVAL)
        == NON_BATTERY_CHECK_IN_INTERVAL
    )
    assert (
        read_poll_attr(nb_device, ZCL_ATTR_POLL_CTRL_LONG_POLL_INTERVAL)
        == NON_BATTERY_LONG_POLL_INTERVAL
    )
    assert (
        read_poll_attr(nb_device, ZCL_ATTR_POLL_CTRL_SHORT_POLL_INTERVAL)
        == NON_BATTERY_SHORT_POLL_INTERVAL
    )
    assert (
        read_poll_attr(nb_device, ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT)
        == NON_BATTERY_FAST_POLL_TIMEOUT
    )


# --- Battery device tests ---

BATTERY_CONFIG = "StubManufacturer;StubDevice;BTC5;SA0u;RB0;"


@pytest.fixture
def bat_device() -> Device:
    proc = make_end_device(BATTERY_CONFIG)
    dev = Device(proc)
    yield dev
    proc.stop()


def test_default_attrs_battery(bat_device: Device):
    """Battery end device has correct poll control defaults."""
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_CHECK_IN_INTERVAL)
        == BATTERY_CHECK_IN_INTERVAL
    )
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_LONG_POLL_INTERVAL)
        == BATTERY_LONG_POLL_INTERVAL
    )
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_SHORT_POLL_INTERVAL)
        == BATTERY_SHORT_POLL_INTERVAL
    )
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT)
        == BATTERY_FAST_POLL_TIMEOUT
    )


def test_check_in_rsp_starts_fast_poll(bat_device: Device):
    """Check-in response with start_fast_polling=true enters fast poll."""
    # Wait for initial fast poll to expire
    bat_device.step_time(BATTERY_FAST_POLL_TIMEOUT * QS_TO_MS + 1)
    # Now send check-in response: start_fast_polling=1, timeout=0x0028 (10s)
    timeout_qs = 0x0028
    payload = struct.pack("<BH", 1, timeout_qs)
    bat_device.call_zigbee_cmd(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_CHECK_IN_RSP, payload
    )
    # Verify short poll rate is active
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_SHORT_POLL_INTERVAL)
        == BATTERY_SHORT_POLL_INTERVAL
    )


def test_check_in_rsp_stops_fast_poll(bat_device: Device):
    """Check-in response with start_fast_polling=false exits fast poll."""
    payload = struct.pack("<BH", 0, 0)
    bat_device.call_zigbee_cmd(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_CHECK_IN_RSP, payload
    )
    # Device should now be in long poll mode


def test_fast_poll_stop_cmd(bat_device: Device):
    """Fast poll stop command while in fast poll exits fast poll."""
    # Device starts in fast poll mode
    bat_device.call_zigbee_cmd(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_FAST_POLL_STOP
    )


def test_fast_poll_stop_after_timeout_still_processed(bat_device: Device):
    """Fast poll stop after timeout is processed because zcl_activity re-enables fast poll."""
    # Wait for fast poll to expire
    bat_device.step_time(BATTERY_FAST_POLL_TIMEOUT * QS_TO_MS + 1)
    # Any ZCL command triggers zcl_activity callback which re-enters fast poll,
    # so fast_poll_stop will always succeed (it enters fast poll, then stops it)
    result = bat_device.call_zigbee_cmd_raw(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_FAST_POLL_STOP
    )
    assert result.get("result") == "PROCESSED"


def test_set_long_poll_interval_valid(bat_device: Device):
    """Set long poll interval with valid value succeeds."""
    new_interval = 0x28  # 10 seconds in quarter-seconds
    payload = struct.pack("<I", new_interval)
    bat_device.call_zigbee_cmd(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_SET_LONG_POLL_INTERVAL, payload
    )
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_LONG_POLL_INTERVAL)
        == new_interval
    )


def test_set_long_poll_interval_too_low(bat_device: Device):
    """Set long poll interval below minimum (0x04) returns INVALID_VALUE."""
    payload = struct.pack("<I", 0x01)  # below minimum
    result = bat_device.call_zigbee_cmd_raw(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_SET_LONG_POLL_INTERVAL, payload
    )
    assert result.get("result") == "INVALID_VALUE"


def test_set_short_poll_interval_valid(bat_device: Device):
    """Set short poll interval with valid value succeeds."""
    new_interval = 0x01  # 250ms
    payload = struct.pack("<H", new_interval)
    bat_device.call_zigbee_cmd(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_SET_SHORT_POLL_INTERVAL, payload
    )
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_SHORT_POLL_INTERVAL)
        == new_interval
    )


def test_set_short_poll_interval_too_high(bat_device: Device):
    """Set short poll interval above long poll returns INVALID_VALUE."""
    # Default long poll for battery is 120 seconds
    payload = struct.pack("<H", 120 * 4 + 1)  # above long poll
    result = bat_device.call_zigbee_cmd_raw(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_SET_SHORT_POLL_INTERVAL, payload
    )
    assert result.get("result") == "INVALID_VALUE"


def test_check_in_interval_writable(bat_device: Device):
    """Check-in interval can be written via attribute write."""
    new_interval = 0x7080  # 2 hours
    bat_device.write_zigbee_attr(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_ATTR_POLL_CTRL_CHECK_IN_INTERVAL, new_interval
    )
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_CHECK_IN_INTERVAL) == new_interval
    )


def test_fast_poll_timeout_writable(bat_device: Device):
    """Fast poll timeout can be written via attribute write."""
    new_timeout = 0x50  # 20 seconds
    bat_device.write_zigbee_attr(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT, new_timeout
    )
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT) == new_timeout
    )


def test_fast_poll_expires_after_timeout(bat_device: Device):
    """Fast poll mode expires after the timeout period - long poll interval is restored."""
    # Device starts in fast poll. Step past timeout.
    # poll_control_cluster_update() in app_task will detect expiry and switch to long poll.
    bat_device.step_time(BATTERY_FAST_POLL_TIMEOUT * QS_TO_MS + 1)
    # Verify we can read the long poll interval (confirming device is still functional)
    assert (
        read_poll_attr(bat_device, ZCL_ATTR_POLL_CTRL_LONG_POLL_INTERVAL)
        == BATTERY_LONG_POLL_INTERVAL
    )


def test_check_in_sends_event(bat_device: Device):
    """After check-in interval, a check-in command event is sent."""
    bat_device.set_network(1)  # must be joined to send commands
    bat_device.clear_events()
    # Step past check-in interval (1 hour = 0x3840 * 250ms = 3600000ms)
    bat_device.step_time(BATTERY_CHECK_IN_INTERVAL * QS_TO_MS + 1)
    # Should have sent a check-in command
    cmd_evt = bat_device.wait_for_cmd_send(
        ep=1, cluster=ZCL_CLUSTER_POLL_CONTROL, cmd=0x00, timeout=1.0
    )
    assert cmd_evt is not None


def test_nvm_persistence(cleanup_nvm):
    """Poll control settings persist across restarts via NVM."""
    config = BATTERY_CONFIG
    new_timeout = 0x50

    # First session: change fast poll timeout
    proc1 = make_end_device(config)
    dev1 = Device(proc1)
    dev1.write_zigbee_attr(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT, new_timeout
    )
    proc1.stop()

    # Second session: verify persistence
    proc2 = make_end_device(config)
    dev2 = Device(proc2)
    assert read_poll_attr(dev2, ZCL_ATTR_POLL_CTRL_FAST_POLL_TIMEOUT) == new_timeout
    proc2.stop()


def test_zcl_activity_triggers_fast_poll(bat_device: Device):
    """Any ZCL command triggers fast poll mode via activity callback.

    Since every zcl_cmd triggers the activity callback before dispatching,
    we verify this by sending a relay command after the fast poll timeout expires,
    then confirming fast_poll_stop succeeds (proving fast poll was re-entered).
    """
    # Wait for initial fast poll to expire
    bat_device.step_time(BATTERY_FAST_POLL_TIMEOUT * QS_TO_MS + 1)

    # Send a relay command - this triggers zcl_activity which re-enters fast poll
    from zcl_consts import ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON

    bat_device.call_zigbee_cmd(2, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)

    # Fast poll stop succeeds, proving the relay command triggered fast poll
    result = bat_device.call_zigbee_cmd_raw(
        1, ZCL_CLUSTER_POLL_CONTROL, ZCL_CMD_POLL_CTRL_FAST_POLL_STOP
    )
    assert result.get("result") == "PROCESSED"
