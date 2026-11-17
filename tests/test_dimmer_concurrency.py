"""Wall switch used while Zigbee commands are in progress (ZDMS16-2).

The MCU handles the wall switch itself and only reports the result (DP
reports, 0x06). These tests inject such reports while a Zigbee write waits
for its ack, while a ramp runs and during the fade hold-off, and check that
the firmware ends up showing what the MCU reported last, without fighting it.
"""
from conftest import Device
from test_dimmer_mcu import (
    DP_LEVEL_L1,
    DP_ON_L1,
    HOLDOFF_MS,
    RAMP_PERIOD_MS,
    ZCL_ATTR_ONOFF,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
    ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    ZCL_LEVEL_MOVE_UP,
    bool_dp,
    dp_writes,
    level,
    mcu_report,
    onoff,
    pump,
    set_level,
    start,
    take_tx,
    tuya_to_zcl,
    value_dp,
    zcl_to_tuya,
)


def _writes_after(proc, device, ms: int, steps: int = 4) -> list[tuple[int, int]]:
    out = []
    for _ in range(steps):
        device.step_time(ms)
        pump(proc)
        out += dp_writes(take_tx(proc))
    return out


def test_wall_off_while_zigbee_level_write_waits_for_ack():
    proc, device = start()
    try:
        set_level(proc, device, 50)
        # Zigbee: brightness 150 while on. "on" is acked at once, the level
        # write waits (the MCU answers it only after the fade).
        device.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL,
                               ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF, bytes([150, 0, 0]))
        pump(proc)
        first = dp_writes(take_tx(proc))
        assert first == [(DP_ON_L1, 1)]
        mcu_report(proc, bool_dp(DP_ON_L1, 1))           # ack of on -> level goes out
        assert dp_writes(take_tx(proc)) == [(DP_LEVEL_L1, zcl_to_tuya(150))]
        mcu_report(proc, bool_dp(DP_ON_L1, 0))           # wall switch: off
        assert onoff(device) == 0
        later = _writes_after(proc, device, 300)
        assert (DP_ON_L1, 1) not in later, f"firmware switched the light back on: {later}"
    finally:
        proc.stop()


def test_wall_off_stops_zigbee_ramp():
    proc, device = start()
    try:
        set_level(proc, device, 60)
        device.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
                               bytes([ZCL_LEVEL_MOVE_UP, 0xFF]))
        _writes_after(proc, device, RAMP_PERIOD_MS + 10, steps=2)
        mcu_report(proc, bool_dp(DP_ON_L1, 0))           # wall switch: off mid-ramp
        assert onoff(device) == 0
        later = _writes_after(proc, device, RAMP_PERIOD_MS + 10, steps=5)
        assert not [w for w in later if w[0] == DP_LEVEL_L1], f"ramp kept writing: {later}"
    finally:
        proc.stop()


def test_wall_dimming_during_hold_off_wins_after_it():
    proc, device = start()
    try:
        set_level(proc, device, 200)
        device.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL,
                               ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF, bytes([180, 0, 0]))
        _writes_after(proc, device, 50, steps=2)
        # wall switch held: the MCU dims to 300 and reports while our fade
        # hold-off still runs
        mcu_report(proc, value_dp(DP_LEVEL_L1, 300))
        assert level(device) == 180                       # held back for now
        device.step_time(HOLDOFF_MS + 100)
        pump(proc)
        assert level(device) == tuya_to_zcl(300), "MCU's level must win after the hold-off"
    finally:
        proc.stop()


def test_zigbee_on_racing_wall_off_ends_off():
    proc, device = start()
    try:
        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, 0x01)   # Zigbee: on
        pump(proc)
        assert dp_writes(take_tx(proc)) == [(DP_ON_L1, 1)]
        mcu_report(proc, bool_dp(DP_ON_L1, 1))                # MCU did it
        mcu_report(proc, bool_dp(DP_ON_L1, 0))                # wall switch right after
        later = _writes_after(proc, device, 200)
        assert onoff(device) == 0
        assert not later, f"unexpected writes after the wall switch: {later}"
    finally:
        proc.stop()


def test_state_converges_to_last_mcu_report():
    """Interleave Zigbee commands and wall switch reports; when things settle
    the attributes equal what the MCU reported last."""
    proc, device = start()
    try:
        set_level(proc, device, 100)
        device.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL,
                               ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF, bytes([220, 0, 0]))
        mcu_report(proc, value_dp(DP_LEVEL_L1, 450))          # wall dims meanwhile
        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, 0x00)   # Zigbee: off
        mcu_report(proc, bool_dp(DP_ON_L1, 1))                # wall: on again
        mcu_report(proc, value_dp(DP_LEVEL_L1, 520))          # wall: dims to 520
        _writes_after(proc, device, 200, steps=3)
        device.step_time(HOLDOFF_MS + 200)
        pump(proc)
        take_tx(proc)
        mcu_report(proc, bool_dp(DP_ON_L1, 1))                # MCU's final word
        mcu_report(proc, value_dp(DP_LEVEL_L1, 520))
        device.step_time(HOLDOFF_MS + 200)
        pump(proc)
        assert onoff(device) == 1
        assert level(device) == tuya_to_zcl(520)
    finally:
        proc.stop()


def test_wall_switch_change_is_reported_to_coordinator():
    proc, device = start()
    try:
        device.clear_events()
        mcu_report(proc, bool_dp(DP_ON_L1, 1))                # wall: on
        device.wait_for_attr_change(1, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF)
    finally:
        proc.stop()
