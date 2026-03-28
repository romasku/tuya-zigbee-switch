"""Tests for indicator LED flash on button press/release."""

from typing import Iterator

import pytest

from tests.client import StubProc
from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF,
    ZCL_ATTR_ONOFF_INDICATOR_MODE,
    ZCL_CLUSTER_ON_OFF,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_INDICATOR_MODE_SAME,
)


@pytest.fixture()
def indicator_device() -> Iterator[Device]:
    # One switch, one relay, indicator LED on A1
    cfg = "X;Y;SA0u;RB0;IA1;"
    with StubProc(device_config=cfg) as proc:
        yield Device(proc)


def test_no_flash_when_relay_attached(indicator_device: Device) -> None:
    """Indicator must follow relay state without spurious blink when relay is attached."""
    relay_ep = 2

    indicator_device.write_zigbee_attr(
        relay_ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF_INDICATOR_MODE,
        ZCL_ONOFF_INDICATOR_MODE_SAME,
    )
    # Turn relay ON via button press (toggle mode: press=ON, release=OFF)
    indicator_device.press_button("A0")
    # Indicator should be ON (following relay)
    assert indicator_device.get_gpio("A1", refresh=True)

    # Wait long enough for a blink cycle to finish (if one were started)
    indicator_device.step_time(200)
    # Indicator must still follow relay (ON), not be turned off by a stale blink
    assert indicator_device.get_gpio("A1", refresh=True)
    assert indicator_device.read_zigbee_attr(relay_ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) == "1"

    indicator_device.release_button("A0")


def test_flash_in_detached_mode(indicator_device: Device) -> None:
    """Indicator should briefly flash on button press when relay is detached."""
    relay_ep = 2

    indicator_device.write_zigbee_attr(
        relay_ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF_INDICATOR_MODE,
        ZCL_ONOFF_INDICATOR_MODE_SAME,
    )
    # Detach relay from button
    indicator_device.zcl_switch_relay_mode_set(1, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED)

    # Relay is OFF, indicator is OFF
    assert not indicator_device.get_gpio("A1", refresh=True)

    # Press button — should start a flash (LED momentarily ON)
    indicator_device.press_button("A0")
    assert indicator_device.get_gpio("A1", refresh=True)

    # After blink finishes, LED should be back OFF
    indicator_device.step_time(200)
    assert not indicator_device.get_gpio("A1", refresh=True)

    indicator_device.release_button("A0")
