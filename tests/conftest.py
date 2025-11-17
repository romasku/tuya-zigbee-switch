import os
import shutil
import time
from dataclasses import dataclass
from typing import Callable, Iterator, TypeVar

import pytest
from client import Event, StubProc

from tests.zcl_consts import (
    ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
    ZCL_ATTR_ONOFF,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
    ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
)

DEBOUNCE_MS = 50


@pytest.fixture(autouse=True)
def cleanup_nvm():
    nvm_dir = "./stub_nvm_data"
    if os.path.exists(nvm_dir):
        shutil.rmtree(nvm_dir)
    yield
    if os.path.exists(nvm_dir):
        shutil.rmtree(nvm_dir)


@pytest.fixture
def device_config() -> str:
    return "StubManufacturer;StubDevice;LC0;SA0u;SA1u;SA2u;SA3u;RB0;RB1;RB2;RB3;"


@pytest.fixture()
def button_pins(device_config: str) -> list[str]:
    parts = device_config.split(";")
    pins = []
    for part in parts[3:]:
        if part.startswith("S"):
            pins.append(part[1:3])
    return pins


@pytest.fixture()
def button_pin(button_pins: list[str]) -> str:
    return button_pins[0]


@pytest.fixture()
def relay_pins(device_config: str) -> list[str]:
    parts = device_config.split(";")
    pins = []
    for part in parts[3:]:
        if part.startswith("R"):
            pins.append(part[1:3])
    return pins


@pytest.fixture()
def relay_pin(relay_pins: list[str]) -> str:
    return relay_pins[0]


@pytest.fixture()
def led_pin(device_config: str) -> str | None:
    parts = device_config.split(";")
    for part in parts[3:]:
        if part.startswith("L"):
            return part[1:3]
    return None


@dataclass
class RelayButtonPair:
    relay_pin: str
    button_pin: str
    switch_endpoint: int
    relay_endpoint: int


@pytest.fixture()
def relay_button_pairs(
    button_pins: list[str], relay_pins: list[str]
) -> list[RelayButtonPair]:
    pairs = []
    for i in range(min(len(button_pins), len(relay_pins))):
        pairs.append(
            RelayButtonPair(
                relay_pin=relay_pins[i],
                button_pin=button_pins[i],
                switch_endpoint=i + 1,
                relay_endpoint=len(button_pins) + i + 1,
            )
        )
    return pairs


@pytest.fixture()
def relay_button_pair(relay_button_pairs: list[RelayButtonPair]) -> RelayButtonPair:
    return relay_button_pairs[0]


@pytest.fixture()
def stub_proc(device_config: str) -> Iterator[StubProc]:
    proc = StubProc(device_config=device_config).start()
    yield proc
    proc.stop()


@dataclass
class ZCLCommandEvent:
    ep: int
    cluster: int
    cmd: int
    data: bytes

    @classmethod
    def from_event(cls, evt: Event) -> "ZCLCommandEvent":
        return cls(
            ep=int(evt.payload["ep"]),
            cluster=int(evt.payload["cluster"], 16),
            cmd=int(evt.payload["cmd"], 16),
            data=bytes.fromhex(evt.payload["data_hex"]),
        )


class Device:
    def __init__(self, p: StubProc) -> None:
        self.p = p
        self._gpio_state: dict[int, bool] = {}
        self._events: list[Event] = []
        self.p.on_event.append(self._evt_parser)

    def get_gpio(self, pin: str, refresh: bool = False) -> bool:
        pin_num = self._parse_pin(pin)
        if refresh or pin_num not in self._gpio_state:
            # query initial state
            res = self.p.exec(f"read_pin {pin_num}")
            assert res.ok, f"GPIO get failed: {res.payload}"
            val = int(res.payload.get("value", "-1"))
            assert val in (0, 1), f"Invalid GPIO value: {res.payload}"
            self._gpio_state[pin_num] = bool(val)
        return self._gpio_state[pin_num]

    def set_gpio(self, pin: str, val: int) -> None:
        res = self.p.exec(f"set_pin {self._parse_pin(pin)} {val}")
        assert res.ok, f"GPIO failed: {res.payload}"

    def press_button(
        self,
        pin: str,
    ) -> None:
        self.set_gpio(pin, 0)  # Low is pressed
        self.step_time(DEBOUNCE_MS + 10)

    def release_button(self, pin: str) -> None:
        self.set_gpio(pin, 1)  # High is released
        self.step_time(DEBOUNCE_MS + 10)

    def click_button(self, pin: str) -> None:
        self.press_button(pin)
        self.release_button(pin)

    def long_click_button(self, pin: str, duration_ms: int = 1000) -> None:
        self.press_button(pin)
        self.step_time(duration_ms)
        self.release_button(pin)

    def status(self) -> dict[str, str]:
        res = self.p.exec("s")
        assert res.ok
        return res.payload

    def read_zigbee_attr(self, endpoint: int, cluster: int, attr: int) -> str:
        res = self.p.exec(f"zcl_read {endpoint} 0x{cluster:04X} 0x{attr:04X}")
        assert res.ok
        return res.payload["value"]

    def write_zigbee_attr(
        self, endpoint: int, cluster: int, attr: int, value: int | str
    ) -> dict[str, str]:
        res = self.p.exec(f"zcl_write {endpoint} 0x{cluster:04X} 0x{attr:04X} {value}")
        assert res.ok
        return res.payload

    def call_zigbee_cmd(self, endpoint: int, cluster: int, cmd: int) -> dict[str, str]:
        res = self.p.exec(f"zcl_cmd {endpoint} 0x{cluster:04X} 0x{cmd:02X}")
        assert res.ok
        return res.payload

    def freeze_time(self) -> None:
        res = self.p.exec("freeze_time 1")
        assert res.ok, f"Freeze time failed: {res.payload}"

    def unfreeze_time(self) -> None:
        res = self.p.exec("freeze_time 0")
        assert res.ok, f"Unfreeze time failed: {res.payload}"

    def step_time(self, ms: int) -> None:
        res = self.p.exec(f"step_time {ms}")
        assert res.ok, f"Step time failed: {res.payload}"

    def _evt_parser(self, evt: Event) -> None:
        if evt.kind == "gpio":
            pin = int(evt.payload.get("pin", "-1"))
            val = int(evt.payload.get("value", "-1"))
            if pin >= 0 and val in (0, 1):
                self._gpio_state[pin] = bool(val)
        # store all events for later queries
        self._events.append(evt)

    def _parse_pin(self, pin: str) -> int:
        port = pin[0].upper()
        number = int(pin[1:])
        port_offset = (ord(port) - ord("A")) * 16
        return port_offset + number

    # --- Zigbee helpers ---
    def is_joined(self) -> bool:
        return self.status().get("joined") == "1"

    def set_network(self, network_state: int) -> None:
        res = self.p.exec(f"net {network_state}")
        assert res.ok

    def clear_events(self) -> None:
        self._events.clear()

    def wait_for_attr_change(
        self,
        ep: int,
        cluster: int,
        attr: int,
        timeout: float = 2.0,
        interval: float = 0.05,
    ) -> None:
        def _has_attr_change() -> bool:
            for e in self._events:
                if e.kind != "zcl_attr_change":
                    continue
                e_ep = int(e.payload["ep"])
                e_cluster = int(e.payload["cluster"], 16)
                e_attr = int(e.payload["attr"], 16)
                if (e_ep, e_cluster, e_attr) == (ep, cluster, attr):
                    return True
            return False

        wait_for(_has_attr_change, timeout=timeout, interval=interval)

    def wait_for_cmd_send(
        self,
        ep: int,
        cluster: int,
        cmd: int | None = None,
        timeout: float = 2.0,
        interval: float = 0.05,
    ) -> ZCLCommandEvent:
        def _find_event() -> ZCLCommandEvent | None:
            for e in self._events:
                if e.kind != "zcl_cmd_send":
                    continue
                event = ZCLCommandEvent.from_event(e)
                if (
                    event.ep == ep
                    and event.cluster == cluster
                    and (cmd is None or event.cmd == cmd)
                ):
                    return event
            return None

        return wait_not_none(_find_event, timeout=timeout, interval=interval)

    def wait_for_announce(
        self,
        timeout: float = 2.0,
        interval: float = 0.05,
    ) -> None:
        def _announce_sent() -> bool:
            for e in self._events:
                if e.kind == "zdo_announce":
                    return True
            return False

        wait_for(_announce_sent, timeout=timeout, interval=interval)

    def zcl_list_cmds(
        self, endpoint: int | None = None, cluster: int | None = None
    ) -> list[ZCLCommandEvent]:
        res = []
        for evt in self._events:
            if evt.kind != "zcl_cmd_send":
                continue
            e = ZCLCommandEvent.from_event(evt)
            if (endpoint is None or e.ep == endpoint) and (
                cluster is None or e.cluster == cluster
            ):
                res.append(e)
        return res

    # Relay helpers:

    def zcl_relay_on(self, endpoint: int) -> None:
        self.call_zigbee_cmd(endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)

    def zcl_relay_off(self, endpoint: int) -> None:
        self.call_zigbee_cmd(endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF)

    def zcl_relay_get(self, endpoint: int) -> str:
        return self.read_zigbee_attr(endpoint, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF)

    # Switch helpers:
    def zcl_switch_mode_set(self, endpoint: int, switch_mode: int) -> None:
        self.write_zigbee_attr(
            endpoint,
            ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
            ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
            switch_mode,
        )

    def zcl_switch_actions_set(self, endpoint: int, actions: int) -> None:
        self.write_zigbee_attr(
            endpoint,
            ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
            ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_ACTIONS,
            actions,
        )

    def zcl_switch_relay_mode_set(self, endpoint: int, relay_mode: int) -> None:
        self.write_zigbee_attr(
            endpoint,
            ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
            ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE,
            relay_mode,
        )

    def zcl_switch_binding_mode_set(self, endpoint: int, binding_mode: int) -> None:
        self.write_zigbee_attr(
            endpoint,
            ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
            ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_BINDING_MODE,
            binding_mode,
        )

    def zcl_switch_get_multistate_value(self, endpoint: int) -> str:
        return self.read_zigbee_attr(
            endpoint,
            ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
            ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
        )

    def zcl_switch_await_multistate_value_reported_change(self, endpoint: int) -> str:
        self.wait_for_attr_change(
            endpoint,
            ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
            ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
        )
        return self.zcl_switch_get_multistate_value(endpoint)


def wait_for(
    condition_fn: Callable[[], bool], timeout: float = 2.0, interval: float = 0.1
) -> None:
    start = time.time()
    while time.time() - start < timeout:
        if condition_fn():
            return
        time.sleep(interval)
    raise TimeoutError("Condition not met within timeout")


T = TypeVar("T")


def wait_not_none(
    fn: Callable[[], T | None], timeout: float = 2.0, interval: float = 0.1
) -> T:
    start = time.time()
    while time.time() - start < timeout:
        res = fn()
        if res is not None:
            return res
        time.sleep(interval)
    raise TimeoutError("Function did not return non-None within timeout")


def ensure_never_true(condition_fn: Callable[[], bool], timeout: float = 0.05) -> None:
    start = time.time()
    while time.time() - start < timeout:
        if condition_fn():
            raise AssertionError("Condition became true")
        time.sleep(0.05)


@pytest.fixture
def device(stub_proc: StubProc) -> Device:
    dev = Device(stub_proc)
    return dev
