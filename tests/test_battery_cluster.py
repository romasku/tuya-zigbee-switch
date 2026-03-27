import pytest

from client import StubProc
from conftest import Device
from zcl_consts import (
    ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE,
    ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE,
    ZCL_CLUSTER_POWER_CFG,
)

BATTERY_REFRESH_INTERVAL_MS = 300000


@pytest.fixture
def device_config() -> str:
    return "StubManufacturer;StubDevice;BTC5;SA0u;"


def read_voltage(device: Device) -> int:
    return int(device.read_zigbee_attr(1, ZCL_CLUSTER_POWER_CFG, ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE))


def read_percentage(device: Device) -> int:
    return int(device.read_zigbee_attr(1, ZCL_CLUSTER_POWER_CFG, ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE))


def set_voltage_and_refresh(device: Device, mv: int) -> None:
    device.set_battery_voltage(mv)
    device.step_time(BATTERY_REFRESH_INTERVAL_MS + 1)


def test_battery_cluster_present(device: Device):
    """Cluster 0x0001 exists on ep 0 with both attrs readable."""
    assert read_voltage(device) is not None
    assert read_percentage(device) is not None


def test_battery_default_voltage(device: Device):
    """Default voltage attr = 30 (3000mV / 100)."""
    assert read_voltage(device) == 30


def test_battery_default_percentage(device: Device):
    """Default percentage attr = 200 (100% in ZCL 0.5% steps)."""
    assert read_percentage(device) == 200


def test_battery_voltage_after_set(device: Device):
    """Set 2500mV, step past refresh, voltage = 25."""
    set_voltage_and_refresh(device, 2500)
    assert read_voltage(device) == 25


def test_battery_percentage_midrange(device: Device):
    """Set 2500mV -> 50% of range -> percentage = 100."""
    set_voltage_and_refresh(device, 2500)
    assert read_percentage(device) == 100


def test_battery_below_minimum(device: Device):
    """Set 1500mV (below min 2000), percentage = 0."""
    set_voltage_and_refresh(device, 1500)
    assert read_percentage(device) == 0


def test_battery_above_maximum(device: Device):
    """Set 3500mV (above max 3000), percentage = 200."""
    set_voltage_and_refresh(device, 3500)
    assert read_percentage(device) == 200


def test_battery_at_exact_min(device: Device):
    """Set 2000mV (exact min), percentage = 0."""
    set_voltage_and_refresh(device, 2000)
    assert read_percentage(device) == 0


def test_battery_at_exact_max(device: Device):
    """Set 3000mV (exact max), percentage = 200."""
    set_voltage_and_refresh(device, 3000)
    assert read_percentage(device) == 200


def test_battery_refresh_timing(device: Device):
    """Battery values update only after 60s refresh interval."""
    device.set_battery_voltage(2800)
    # Step just before refresh - should still be default
    device.step_time(BATTERY_REFRESH_INTERVAL_MS - 1)
    assert read_voltage(device) == 30  # still default

    # Step past refresh
    device.step_time(2)
    assert read_voltage(device) == 28  # now updated


def test_battery_attr_change_events(device: Device):
    """After refresh, zcl_attr_change events emitted for both attrs."""
    device.set_battery_voltage(2500)
    device.clear_events()
    device.step_time(BATTERY_REFRESH_INTERVAL_MS + 1)

    device.wait_for_attr_change(1, ZCL_CLUSTER_POWER_CFG, ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE)
    device.wait_for_attr_change(1, ZCL_CLUSTER_POWER_CFG, ZCL_ATTR_POWER_CFG_BATTERY_PERCENTAGE)


def test_battery_cluster_absent_without_config():
    """Without BT config entry, cluster 0x0001 is not present."""
    p = StubProc(device_config="StubManufacturer;StubDevice;SA0u;RB0;").start()
    try:
        Device(p)  # ensure boot completes
        res = p.exec(f"zcl_read 1 0x{ZCL_CLUSTER_POWER_CFG:04X} 0x{ZCL_ATTR_POWER_CFG_BATTERY_VOLTAGE:04X}")
        assert not res.ok
    finally:
        p.stop()
