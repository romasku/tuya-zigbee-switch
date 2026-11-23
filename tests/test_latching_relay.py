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
        LatchingRelayTestConfig(on_pin="B3", off_pin="A3", ep=4),
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


@dataclass
class PulseInfo:
    pin: str
    pulse_start_time: int
    pulse_end_time: int | None

    @property
    def duration(self) -> int | None:
        if self.pulse_end_time is None:
            return None
        return self.pulse_end_time - self.pulse_start_time


class PulseTracker:
    def __init__(self, gpios: list[str]) -> None:
        self.pulses: list[PulseInfo] = []
        self.current_time = 0
        self.gpios = gpios

    def reset(self) -> None:
        self.pulses = []

    def ended_pulses(self) -> list[PulseInfo]:
        return [p for p in self.pulses if p.pulse_end_time is not None]

    def active_pulses(self) -> list[PulseInfo]:
        return [p for p in self.pulses if p.pulse_end_time is None]

    def refresh(self, time_passed: int, device: Device) -> None:
        self.current_time += time_passed
        for pin in self.gpios:
            pin_state = device.get_gpio(pin)
            pulse = next(
                (p for p in self.pulses if p.pin == pin and p.pulse_end_time is None),
                None,
            )
            if pin_state is True and pulse is None:
                # New pulse started
                self.pulses.append(
                    PulseInfo(
                        pin=pin, pulse_start_time=self.current_time, pulse_end_time=None
                    )
                )
            elif pin_state is False and pulse is not None:
                # Pulse ended
                pulse.pulse_end_time = self.current_time


def test_mutual_exclusion_between_relays(
    latching_device: Device, pins_config: list[LatchingRelayTestConfig]
) -> None:
    def toggle_all_relays(cmd: int) -> None:
        for cfg in pins_config:
            latching_device.call_zigbee_cmd(cfg.ep, ZCL_CLUSTER_ON_OFF, cmd)

    tracker = PulseTracker(
        gpios=[cfg.on_pin for cfg in pins_config] + [cfg.off_pin for cfg in pins_config]
    )

    for iteration in range(10):
        tracker.reset()
        toggle_all_relays(0x01 if iteration % 2 == 0 else 0x00)
        tracker.refresh(0, latching_device)

        while len(tracker.ended_pulses()) < len(pins_config):
            latching_device.step_time(10)
            tracker.refresh(10, latching_device)
            assert len(tracker.active_pulses()) <= 1

        for pulse in tracker.ended_pulses():
            assert pulse.duration is not None
            assert 80 <= pulse.duration <= 120


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
    assert time_passed <= 120  # No longer than a single pulse


# A0 is encoded as 0, so this tests that it works correctly
def test_off_pin_is_a0() -> None:
    cfg = "X;Y;RC2A0;"
    p = StubProc(device_config=cfg).start()
    try:
        device = Device(p)

        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, 0x01)

        time_passed = 0

        while device.get_gpio("C2") is True:
            device.step_time(10)
            time_passed += 10
            assert time_passed < 1000, "Timed out waiting for ON pulse to end"

        time_passed = 0

        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, 0x00)

        while device.get_gpio("A0") is True:
            device.step_time(10)
            time_passed += 10
            assert time_passed < 1000, "Timed out waiting for ON pulse to end"

    finally:
        p.stop()
