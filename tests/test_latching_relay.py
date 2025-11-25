from dataclasses import dataclass
from typing import Iterator

import pytest

from tests.client import StubProc
from tests.conftest import Device, wait_for
from tests.zcl_consts import ZCL_CLUSTER_ON_OFF


@dataclass
class LatchingRelayTestConfig:
    on_pin: str
    off_pin: str
    ep: int


@pytest.fixture()
def pins_config() -> list[LatchingRelayTestConfig]:
    return [
        LatchingRelayTestConfig(on_pin="B0", off_pin="C0", ep=1),
        LatchingRelayTestConfig(on_pin="B1", off_pin="C1", ep=2),
        LatchingRelayTestConfig(on_pin="B2", off_pin="C2", ep=3),
        LatchingRelayTestConfig(on_pin="B3", off_pin="C3", ep=4),
    ]


@pytest.fixture()
def latching_device(pins_config: list[LatchingRelayTestConfig]) -> Iterator[Device]:
    cfg = "X;Y;" + ";".join(f"R{cfg.on_pin}{cfg.off_pin}" for cfg in pins_config) + ";"
    p = StubProc(device_config=cfg).start()
    try:
        yield Device(p)
    finally:
        p.stop()


@pytest.fixture()
def latching_simultenious_device(
    pins_config: list[LatchingRelayTestConfig],
) -> Iterator[Device]:
    cfg = (
        "X;Y;SLP;"
        + ";".join(f"R{cfg.on_pin}{cfg.off_pin}" for cfg in pins_config)
        + ";"
    )
    p = StubProc(device_config=cfg).start()
    try:
        yield Device(p)
    finally:
        p.stop()


def count_pins_high(device: Device, pins_config: list[LatchingRelayTestConfig]) -> int:
    return sum(1 for cfg in pins_config if device.get_gpio(cfg.on_pin) is True) + sum(
        1 for cfg in pins_config if device.get_gpio(cfg.off_pin) is True
    )


def test_on_pulse(
    latching_device: Device, pins_config: list[LatchingRelayTestConfig]
) -> None:
    cfg = pins_config[0]
    latching_device.call_zigbee_cmd(cfg.ep, ZCL_CLUSTER_ON_OFF, 0x01)
    wait_for(lambda: latching_device.get_gpio(cfg.on_pin) is True)
    assert latching_device.get_gpio(cfg.off_pin) is False
    duration = 0
    while latching_device.get_gpio(cfg.on_pin) is True:
        duration += 1
        latching_device.step_time(1)
    assert 50 <= duration <= 200


def test_off_pulse(
    latching_device: Device, pins_config: list[LatchingRelayTestConfig]
) -> None:
    cfg = pins_config[0]
    latching_device.call_zigbee_cmd(cfg.ep, ZCL_CLUSTER_ON_OFF, 0x00)
    wait_for(lambda: latching_device.get_gpio(cfg.off_pin) is True)
    assert latching_device.get_gpio(cfg.on_pin) is False
    duration = 0
    while latching_device.get_gpio(cfg.off_pin) is True:
        duration += 1
        latching_device.step_time(1)
    assert 50 <= duration <= 200


def test_mutual_exclusion_between_pins(
    latching_device: Device, pins_config: list[LatchingRelayTestConfig]
) -> None:
    for cfg in pins_config:
        latching_device.call_zigbee_cmd(cfg.ep, ZCL_CLUSTER_ON_OFF, 0x01)
        wait_for(lambda: latching_device.get_gpio(cfg.on_pin) is True)
        assert latching_device.get_gpio(cfg.off_pin) is False
        latching_device.step_time(200)  # Allow pulse to finish

        latching_device.call_zigbee_cmd(cfg.ep, ZCL_CLUSTER_ON_OFF, 0x00)
        wait_for(lambda: latching_device.get_gpio(cfg.on_pin) is False)
        assert latching_device.get_gpio(cfg.off_pin) is True
        latching_device.step_time(200)  # Allow pulse to finish


def test_mutual_exclusion_between_relays_all_on(
    latching_device: Device, pins_config: list[LatchingRelayTestConfig]
) -> None:
    for cfg in pins_config:
        latching_device.call_zigbee_cmd(cfg.ep, ZCL_CLUSTER_ON_OFF, 0x01)

    relays_activated = [False] * len(pins_config)
    while not all(relays_activated):
        for i, cfg in enumerate(pins_config):
            if not relays_activated[i]:
                if latching_device.get_gpio(cfg.on_pin) is True:
                    relays_activated[i] = True
        assert count_pins_high(latching_device, pins_config) <= 1
        latching_device.step_time(10)


def test_mutual_exclusion_between_relays_all_off(
    latching_device: Device, pins_config: list[LatchingRelayTestConfig]
) -> None:
    for cfg in pins_config:
        latching_device.call_zigbee_cmd(cfg.ep, ZCL_CLUSTER_ON_OFF, 0x00)

    relays_activated = [False] * len(pins_config)
    time_passed = 0
    while not all(relays_activated):
        for i, cfg in enumerate(pins_config):
            if not relays_activated[i]:
                if latching_device.get_gpio(cfg.off_pin) is True:
                    relays_activated[i] = True
        assert count_pins_high(latching_device, pins_config) <= 1
        latching_device.step_time(10)
        time_passed += 10
    assert time_passed > 200  # Longer than a single pulse


def test_mutual_no_exclusion_between_relays_all_on(
    latching_simultenious_device: Device, pins_config: list[LatchingRelayTestConfig]
) -> None:
    for cfg in pins_config:
        latching_simultenious_device.call_zigbee_cmd(cfg.ep, ZCL_CLUSTER_ON_OFF, 0x01)
    relays_activated = [False] * len(pins_config)
    time_passed = 0
    while not all(relays_activated):
        for i, cfg in enumerate(pins_config):
            if not relays_activated[i]:
                if latching_simultenious_device.get_gpio(cfg.on_pin) is True:
                    relays_activated[i] = True
        latching_simultenious_device.step_time(10)
        time_passed += 10
    assert time_passed <= 200  # No longer than a single pulse
