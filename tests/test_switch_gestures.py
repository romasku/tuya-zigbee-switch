import struct
from pathlib import Path

import pytest
from client import Event, StubProc

from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_CONFIRM_RELEASE_MS as CONFIRM,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MAX_PRESS_COUNT as MAX_COUNT,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE as MODE,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_RELAY_MODE as RELAY_MODE,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG as SWITCH_CFG,
    ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    ZCL_CMD_ONOFF_TOGGLE,
    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT,
    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED,
    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY,
    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE,
)

GESTURE_ATTR = 0xFF00


def collect_reports(proc: StubProc) -> list[tuple[int, int]]:
    reports = []

    def collect(event: Event):
        if event.kind != "zcl_attr_report":
            return
        data = event.payload
        if (
            int(data["cluster"], 16) == ZCL_CLUSTER_MULTISTATE_INPUT_BASIC
            and int(data["attr"], 16) == GESTURE_ATTR
        ):
            assert int(data["type"], 16) == 0x21
            (value,) = struct.unpack("<H", bytes.fromhex(data["data_hex"]))
            reports.append((int(data["ep"]), value))

    proc.on_event.append(collect)
    return reports


def setup_button(device: Device, endpoint: int, limit: int):
    device.zcl_switch_mode_set(endpoint, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY)
    device.write_zigbee_attr(endpoint, SWITCH_CFG, MAX_COUNT, limit)


@pytest.mark.parametrize("clicks,limit,code", [(1, 2, 5), (2, 2, 7), (3, 3, 10), (5, 3, 10)])
def test_finalized_click_report(device, button_pin, clicks, limit, code):
    setup_button(device, 1, limit)
    reports = collect_reports(device.p)
    for _ in range(clicks):
        device.click_button(button_pin)
    device.step_time(199)
    # ZCL read also gives the simulator a turn to process scheduled tasks.
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == []
    device.step_time(1)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == [(1, code)]
    assert device.read_zigbee_attr(1, ZCL_CLUSTER_MULTISTATE_INPUT_BASIC, GESTURE_ATTR) == "0"
    device.step_time(500)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == [(1, code)]


def test_window_and_endpoint_isolation(device, button_pins):
    setup_button(device, 1, 2)
    setup_button(device, 2, 3)
    device.write_zigbee_attr(1, SWITCH_CFG, CONFIRM, 350)
    reports = collect_reports(device.p)
    first, second = button_pins[:2]
    device.press_button(first)
    device.press_button(second)
    device.release_button(first)
    device.release_button(second)  # EP1 released 60 ms earlier, but has a longer window.
    device.step_time(200)
    assert device.zcl_switch_get_multistate_value(2) == "0"
    assert reports == [(2, 5)]
    device.step_time(90)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == [(2, 5), (1, 5)]
    device.click_button(first)  # Outside the previous sequence.
    device.step_time(350)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == [(2, 5), (1, 5), (1, 5)]


@pytest.mark.parametrize("taps,hold,release", [(0, 2, 6), (1, 8, 9), (2, 11, 12)])
def test_hold_uses_existing_move_stop(device, button_pin, taps, hold, release):
    setup_button(device, 1, 3)
    reports = collect_reports(device.p)
    for _ in range(taps):
        device.click_button(button_pin)
    device.press_button(button_pin)
    device.step_time(900)
    assert device.zcl_switch_get_multistate_value(1) == str(hold)
    assert reports == [(1, hold)]  # No premature click when the last press becomes a hold.
    # Debounced release only 60 ms after the hold callback (within Telink's
    # 100 ms deferred reporting window). Both direct reports must survive.
    device.release_button(button_pin)
    device.step_time(400)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == [(1, hold), (1, release)]
    assert [cmd.cmd for cmd in device.zcl_list_cmds(1, ZCL_CLUSTER_LEVEL_CONTROL)] == [
        ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF, ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    ]


@pytest.mark.parametrize("limit", [1, 2])
def test_avatto_end_device_short_binding_timing(limit):
    config = "hbxsdd6k;TS0011-avatto;BB4u;LB5;SC0u;RC2;"
    with StubProc(cmd=["./build/stub/stub_end_device"], device_config=config) as proc:
        device = Device(proc)
        device.set_gpio("C0", 1)  # The stub pre-initializes only A0..A3 high.
        device.step_time(60)
        setup_button(device, 1, limit)
        device.zcl_switch_relay_mode_set(1, ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED)
        device.zcl_switch_actions_set(1, ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE)
        device.zcl_switch_binding_mode_set(1, ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT)
        device.clear_events()
        reports = collect_reports(proc)

        for i in range(limit):
            device.press_button("C0")
            assert device.zcl_switch_get_multistate_value(1) == "1"
            assert len(device.zcl_list_cmds(1, ZCL_CLUSTER_ON_OFF)) == i
            device.release_button("C0")
            assert device.zcl_switch_get_multistate_value(1) == "0"
            assert [cmd.cmd for cmd in device.zcl_list_cmds(1, ZCL_CLUSTER_ON_OFF)] == [
                ZCL_CMD_ONOFF_TOGGLE
            ] * (i + 1)

        assert device.zcl_relay_get(2) == "0"
        assert reports == ([(1, 5)] if limit == 1 else [])
        device.step_time(200)
        assert device.zcl_switch_get_multistate_value(1) == "0"
        assert reports == [(1, 5 if limit == 1 else 7)]
        assert len(device.zcl_list_cmds(1, ZCL_CLUSTER_ON_OFF)) == limit


def test_config_writes_cancel_pending_confirmation(device, button_pin):
    setup_button(device, 1, 3)
    reports = collect_reports(device.p)
    device.click_button(button_pin)
    device.write_zigbee_attr(1, SWITCH_CFG, MAX_COUNT, 1)
    device.step_time(200)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == []
    device.click_button(button_pin)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == [(1, 5)]

    device.write_zigbee_attr(1, SWITCH_CFG, MAX_COUNT, 2)
    device.click_button(button_pin)
    device.zcl_switch_mode_set(1, ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE)
    device.step_time(200)
    assert device.zcl_switch_get_multistate_value(1) == "4"
    device.click_button(button_pin)
    assert device.zcl_switch_get_multistate_value(1) == "4"
    assert reports == [(1, 5), (1, 3), (1, 4)]


def test_optional_timer_failure_discards_sequence(device, button_pin):
    setup_button(device, 1, 2)
    reports = collect_reports(device.p)
    assert device.p.exec("task_reject 1").ok
    device.click_button(button_pin)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == []
    assert device.p.exec("task_reject 0").ok
    device.click_button(button_pin)
    device.step_time(200)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == [(1, 5)]


def test_unchanged_gesture_config_does_not_write_or_cancel(device, button_pin):
    setup_button(device, 1, 2)
    reports = collect_reports(device.p)
    record = Path("stub_nvm_data/item_23.bin")
    before = record.stat().st_mtime_ns
    device.click_button(button_pin)
    device.write_zigbee_attr(1, SWITCH_CFG, MAX_COUNT, 2)
    device.step_time(200)
    assert device.zcl_switch_get_multistate_value(1) == "0"
    assert reports == [(1, 5)]
    assert record.stat().st_mtime_ns == before


def test_failed_gesture_save_keeps_previous_config(device):
    record = Path("stub_nvm_data/item_23.bin")
    record.mkdir(parents=True)  # Opening this path as a stub NVM file fails.
    device.write_zigbee_attr(1, SWITCH_CFG, MAX_COUNT, 3)
    assert device.read_zigbee_attr(1, SWITCH_CFG, MAX_COUNT) == "1"
    record.rmdir()
    device.write_zigbee_attr(1, SWITCH_CFG, MAX_COUNT, 3)
    assert device.read_zigbee_attr(1, SWITCH_CFG, MAX_COUNT) == "3"


def test_legacy_nv_record_and_gesture_persistence(device_config):
    directory = Path("stub_nvm_data")
    directory.mkdir()
    # The eight-byte switch record at NV item 4 predates gesture configuration.
    legacy = struct.pack("<BBBBHBB", 1, 2, 0, 1, 1250, 32, 3)
    switch_record = directory / "item_04.bin"
    switch_record.write_bytes(legacy)
    with StubProc(device_config=device_config) as proc:
        device = Device(proc)
        assert device.read_zigbee_attr(1, SWITCH_CFG, MODE) == "1"
        assert device.read_zigbee_attr(1, SWITCH_CFG, RELAY_MODE) == "0"
        assert device.read_zigbee_attr(1, SWITCH_CFG, MAX_COUNT) == "1"
        assert device.read_zigbee_attr(1, SWITCH_CFG, CONFIRM) == "200"
        device.write_zigbee_attr(1, SWITCH_CFG, MAX_COUNT, 0)
        device.write_zigbee_attr(1, SWITCH_CFG, CONFIRM, 0)
        assert device.read_zigbee_attr(1, SWITCH_CFG, MAX_COUNT) == "1"
        assert device.read_zigbee_attr(1, SWITCH_CFG, CONFIRM) == "200"
        device.write_zigbee_attr(1, SWITCH_CFG, MAX_COUNT, 254)
        device.write_zigbee_attr(1, SWITCH_CFG, CONFIRM, 3000)
        assert device.read_zigbee_attr(1, SWITCH_CFG, MAX_COUNT) == "3"
        assert device.read_zigbee_attr(1, SWITCH_CFG, CONFIRM) == "2000"
        device.write_zigbee_attr(1, SWITCH_CFG, CONFIRM, 275)

    assert switch_record.read_bytes() == legacy
    with StubProc(device_config=device_config) as proc:
        device = Device(proc)
        assert device.read_zigbee_attr(1, SWITCH_CFG, MODE) == "1"
        assert device.read_zigbee_attr(1, SWITCH_CFG, RELAY_MODE) == "0"
        assert device.read_zigbee_attr(1, SWITCH_CFG, MAX_COUNT) == "3"
        assert device.read_zigbee_attr(1, SWITCH_CFG, CONFIRM) == "275"
        assert device.read_zigbee_attr(2, SWITCH_CFG, MAX_COUNT) == "1"
