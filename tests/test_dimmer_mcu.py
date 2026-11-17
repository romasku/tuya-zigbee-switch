"""Dimmer <-> secondary MCU behaviour for the AVATTO ZDMS16-2 (PY32F002A MCU).

Facts these tests rely on (Tuya schema of the DMS16 family, same DP map on the
WiFi DMS16-W1/W2 and the Zigbee ZDMS16-2):

    DP 1/7   on/off            bool
    DP 2/8   brightness        value, valid range 10..1000
    DP 3/9   minimum brightness value, 10..1000
    DP 4/10  switch type       enum flip/sync/button
    DP 5/11  maximum brightness value, 10..1000
    DP 14    power-on state    enum off/on/memory

The MCU fades internally and reports intermediate brightness values while it
fades (seen with OpenBeken on the WiFi twin). OpenBeken also needed a TX queue
(one frame in flight) to drive this MCU reliably.
"""
import subprocess
import tempfile
from pathlib import Path

from client import StubProc
from conftest import Device
from tuya_mcu import (  # noqa: F401  (re-exported for the other dimmer tests)
    TUYA_CMD_REPORT,
    TUYA_CMD_RESET_NETWORK,
    TUYA_CMD_REPORT_PASSIVE,
    TUYA_CMD_WRITE,
    TUYA_TYPE_BOOL,
    TUYA_TYPE_ENUM,
    TUYA_TYPE_VALUE,
    bool_dp,
    dp_payload,
    dp_units,
    dp_writes,
    enum_dp,
    mcu_report,
    parse_frames,
    pump,
    take_tx,
    tuya_frame,
    value_dp,
)
from zcl_consts import (
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CMD_LEVEL_MOVE,
    ZCL_CMD_LEVEL_MOVE_TO_LEVEL,
    ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
    ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    ZCL_CMD_LEVEL_STEP,
    ZCL_CMD_LEVEL_STEP_WITH_ON_OFF,
    ZCL_CMD_LEVEL_STOP,
    ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
    ZCL_LEVEL_MOVE_DOWN,
    ZCL_LEVEL_MOVE_UP,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ONOFF_TOGGLE,
)

ZDMS16_2 = "jtbgusdc;TS0601-ZDMS16-2;DM2;M;"
# The same DPs spelled out with Pxx/U overrides.
ZDMS16_2_EXPLICIT = "jtbgusdc;TS0601-ZDMS16-2;DM2;P00O01L02S04M03X05;P01O07L08S0AM09X0B;U0E;M;"

ZCL_CLUSTER_BALLAST = 0x0301
ZCL_ATTR_BALLAST_MIN = 0x0010
ZCL_ATTR_BALLAST_MAX = 0x0011
ZCL_ATTR_CURRENT_LEVEL = 0x0000
ZCL_ATTR_ONOFF = 0x0000
ZCL_ATTR_SWITCH_TYPE = 0x0000


DP_ON_L1, DP_LEVEL_L1, DP_MIN_L1, DP_SWITCH_TYPE_L1, DP_MAX_L1 = 1, 2, 3, 4, 5
DP_ON_L2, DP_LEVEL_L2, DP_MIN_L2 = 7, 8, 9

TUYA_BRIGHTNESS_MIN = 10

RAMP_PERIOD_MS = 300
RAMP_STEP = 12

# Longer than any ack wait or fade hold-off the firmware may use.
SETTLE_MS = 6000
HOLDOFF_MS = 3000  # firmware DIMMER_FADE_HOLDOFF_MS


# --------------------------------------------------------------------------
# helpers


def zcl_to_tuya(level: int) -> int:
    return (level * 1000 + 127) // 254


def tuya_to_zcl(value: int) -> int:
    return (value * 254 + 500) // 1000


def scaled_to_tuya(level: int, mn: int, mx: int) -> int:
    """Firmware mapping with level scaling: 1 = MCU min, 254 = MCU max."""
    level = max(1, min(254, level))
    return mn + ((level - 1) * (mx - mn) + 126) // 253


def scaled_to_zcl(value: int, mn: int, mx: int) -> int:
    if value <= mn:
        return 1
    if value >= mx:
        return 254
    return max(1, min(254, 1 + ((value - mn) * 253 + (mx - mn) // 2) // (mx - mn)))


def ack_writes(proc: StubProc, device: Device, rounds: int = 8) -> list[tuple[int, int]]:
    """Play a well-behaved MCU: echo every DP write back unchanged as a 0x06
    report (as the ZDMS16-2 does), until the firmware has nothing more to
    send. Returns all writes seen as (dpid, value)."""
    seen = []
    for _ in range(rounds):
        pump(proc)
        tx = take_tx(proc)
        if not dp_units(tx):
            device.step_time(50)
            pump(proc)
            tx = take_tx(proc)
            if not dp_units(tx):
                break
        seen += dp_writes(tx)
        for unit in dp_units(tx):
            mcu_report(proc, unit)
    return seen


def level(device: Device, ep: int = 1) -> int:
    return int(device.read_zigbee_attr(ep, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_ATTR_CURRENT_LEVEL))


def onoff(device: Device, ep: int = 1) -> int:
    return int(device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF))


def start() -> tuple[StubProc, Device]:
    proc = StubProc(device_config=ZDMS16_2).start()
    device = Device(proc)
    pump(proc)
    take_tx(proc)  # boot chatter
    return proc, device


def set_level(proc: StubProc, device: Device, lvl: int) -> None:
    device.call_zigbee_cmd(
        1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
        bytes([lvl, 0, 0]),
    )
    ack_writes(proc, device)
    device.step_time(SETTLE_MS)
    pump(proc)
    # The first-contact frames (2 s after boot: version query, query-all) are
    # still going out: no MCU here answers them, so let both acks time out
    # before the test goes on.
    for _ in range(2):
        device.step_time(200)
        pump(proc)
        take_tx(proc)


# --------------------------------------------------------------------------
# MCU-side settings must be read back


def test_min_max_reports_reach_ballast_attributes():
    """Answer to query-all carries min/max: the ballast attributes must show the
    MCU's real values, not the parser defaults (max 100/254 = 39 %)."""
    proc, device = start()
    try:
        mcu_report(proc, value_dp(DP_MIN_L1, 100), cmd=TUYA_CMD_REPORT_PASSIVE)
        mcu_report(proc, value_dp(DP_MAX_L1, 600), cmd=TUYA_CMD_REPORT_PASSIVE)
        mcu_report(proc, value_dp(DP_MIN_L2, 200), cmd=TUYA_CMD_REPORT_PASSIVE)
        assert int(device.read_zigbee_attr(1, ZCL_CLUSTER_BALLAST, ZCL_ATTR_BALLAST_MIN)) == tuya_to_zcl(100)
        assert int(device.read_zigbee_attr(1, ZCL_CLUSTER_BALLAST, ZCL_ATTR_BALLAST_MAX)) == tuya_to_zcl(600)
        assert int(device.read_zigbee_attr(2, ZCL_CLUSTER_BALLAST, ZCL_ATTR_BALLAST_MIN)) == tuya_to_zcl(200)
    finally:
        proc.stop()


def test_switch_type_report_reaches_switch_config():
    proc, device = start()
    try:
        mcu_report(proc, enum_dp(DP_SWITCH_TYPE_L1, 2), cmd=TUYA_CMD_REPORT_PASSIVE)
        assert int(device.read_zigbee_attr(
            1, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, ZCL_ATTR_SWITCH_TYPE)) == 2
    finally:
        proc.stop()


def test_no_default_settings_pushed_to_mcu():
    """The MCU stores min/max/switch type itself. The firmware must never
    overwrite them with its own parser defaults, at boot or on first contact."""
    proc, device = start()
    try:
        mcu_report(proc, cmd=0x20)  # MCU asks for network status = first contact
        device.step_time(SETTLE_MS)
        pump(proc)
        written = {dp for dp, _ in dp_writes(take_tx(proc))}
        assert not written & {DP_MIN_L1, DP_MAX_L1, DP_SWITCH_TYPE_L1, DP_MIN_L2, 11, 10}
    finally:
        proc.stop()


# --------------------------------------------------------------------------
# several datapoints in one frame


def test_multi_dp_frame_is_fully_decoded():
    proc, device = start()
    try:
        mcu_report(
            proc,
            bool_dp(DP_ON_L1, 1),
            value_dp(DP_LEVEL_L1, 500),
            bool_dp(DP_ON_L2, 1),
            cmd=TUYA_CMD_REPORT_PASSIVE,
        )
        assert onoff(device, 1) == 1
        assert level(device, 1) == tuya_to_zcl(500)
        assert onoff(device, 2) == 1
    finally:
        proc.stop()


def test_full_status_dump_larger_than_64_bytes():
    """Query-all answer for both channels is ~90 bytes in one frame."""
    proc, device = start()
    try:
        mcu_report(
            proc,
            bool_dp(1, 1), value_dp(2, 400), value_dp(3, 100), enum_dp(4, 2),
            value_dp(5, 900), value_dp(6, 0),
            bool_dp(7, 1), value_dp(8, 700), value_dp(9, 150), enum_dp(10, 2),
            value_dp(11, 800), value_dp(12, 0),
            enum_dp(14, 0),
            cmd=TUYA_CMD_REPORT_PASSIVE,
        )
        # min/max are known from the same dump, so levels are scaled.
        assert level(device, 1) == scaled_to_zcl(400, 100, 900)
        assert level(device, 2) == scaled_to_zcl(700, 150, 800)
        assert int(device.read_zigbee_attr(2, ZCL_CLUSTER_BALLAST, ZCL_ATTR_BALLAST_MAX)) == tuya_to_zcl(800)
    finally:
        proc.stop()


# --------------------------------------------------------------------------
# one frame in flight


def test_one_frame_in_flight_until_mcu_acks():
    proc, device = start()
    try:
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
            bytes([200, 0, 0]),
        )
        pump(proc)
        first = dp_writes(take_tx(proc))
        assert len(first) == 1, f"expected one frame in flight, got {first}"
        dpid, value = first[0]
        mcu_report(proc, value_dp(dpid, value) if dpid == DP_LEVEL_L1 else bool_dp(dpid, value))
        second = dp_writes(take_tx(proc))
        assert len(second) == 1, f"next frame must follow the ack, got {second}"
        assert {first[0][0], second[0][0]} == {DP_ON_L1, DP_LEVEL_L1}
    finally:
        proc.stop()


def test_queue_moves_on_after_ack_timeout():
    """An MCU that never answers must not stall the queue forever."""
    proc, device = start()
    try:
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
            bytes([200, 0, 0]),
        )
        pump(proc)
        assert len(dp_writes(take_tx(proc))) == 1
        device.step_time(500)
        pump(proc)
        assert len(dp_writes(take_tx(proc))) == 1
    finally:
        proc.stop()


def test_on_is_sent_before_level():
    """On first: the MCU acknowledges a level written while the channel is
    off but does not keep it, and "on" restores its remembered level
    (bench 2026-11-16: off, level 236, on -> MCU still at 807)."""
    proc, device = start()
    try:
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
            bytes([200, 0, 0]),
        )
        writes = ack_writes(proc, device)
        assert [dp for dp, _ in writes][:2] == [DP_ON_L1, DP_LEVEL_L1]
    finally:
        proc.stop()


def test_move_to_level_without_onoff_while_off_does_nothing():
    """ZCL: no effect while off (and the Telink stack drops it before us)."""
    proc, device = start()
    try:
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL,
            bytes([128, 0, 0, 0, 0]),
        )
        assert ack_writes(proc, device) == []
        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, 0x01)  # on
        assert ack_writes(proc, device) == [(DP_ON_L1, 1)]
    finally:
        proc.stop()


# --------------------------------------------------------------------------
# level scaling to the MCU's min..max range


def dump_range(proc: StubProc, mn: int, mx: int, ch: int = 1) -> None:
    dps = {1: (DP_MIN_L1, DP_MAX_L1), 2: (DP_MIN_L2, 11)}[ch]
    mcu_report(proc, value_dp(dps[0], mn), value_dp(dps[1], mx), cmd=TUYA_CMD_REPORT_PASSIVE)


def test_scaling_maps_slider_ends_to_mcu_range():
    proc, device = start()
    try:
        dump_range(proc, 200, 1000)
        for lvl, expect in ((1, 200), (254, 1000), (128, scaled_to_tuya(128, 200, 1000))):
            device.call_zigbee_cmd(
                1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
                bytes([lvl, 0, 0]),
            )
            levels = [v for dp, v in ack_writes(proc, device) if dp == DP_LEVEL_L1]
            assert levels[-1:] == [expect], (lvl, levels)
    finally:
        proc.stop()


def test_scaling_maps_mcu_reports_back():
    proc, device = start()
    try:
        dump_range(proc, 200, 1000)
        mcu_report(proc, value_dp(DP_LEVEL_L1, 600))  # wall switch dimming
        assert level(device) == scaled_to_zcl(600, 200, 1000)
        mcu_report(proc, value_dp(DP_LEVEL_L1, 150))  # below the MCU minimum
        assert level(device) == 1
    finally:
        proc.stop()


def test_range_change_rescales_current_level():
    """Same light output, other range: the level attribute must follow."""
    proc, device = start()
    try:
        dump_range(proc, 200, 1000)
        mcu_report(proc, value_dp(DP_LEVEL_L1, 600))
        before = level(device)
        mcu_report(proc, value_dp(DP_MIN_L1, 400))
        after = level(device)
        assert before == scaled_to_zcl(600, 200, 1000)
        assert after == scaled_to_zcl(600, 400, 1000) and after != before
    finally:
        proc.stop()


def test_scaling_waits_for_known_range():
    """Before the MCU told us its range, levels map 1..254 -> 10..1000."""
    proc, device = start()
    try:
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
            bytes([200, 0, 0]),
        )
        levels = [v for dp, v in ack_writes(proc, device) if dp == DP_LEVEL_L1]
        assert levels == [zcl_to_tuya(200)]
    finally:
        proc.stop()


def test_dr_token_keeps_stock_mapping():
    proc = StubProc(device_config=ZDMS16_2.replace(";M;", ";DR;M;")).start()
    device = Device(proc)
    try:
        pump(proc)
        take_tx(proc)
        dump_range(proc, 200, 1000)
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
            bytes([1, 0, 0]),
        )
        levels = [v for dp, v in ack_writes(proc, device) if dp == DP_LEVEL_L1]
        assert levels == [TUYA_BRIGHTNESS_MIN]
    finally:
        proc.stop()


# --------------------------------------------------------------------------
# MCU fade reports must not fight our own target


def test_ramp_ignores_lagging_fade_reports():
    proc, device = start()
    try:
        set_level(proc, device, 60)
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
            bytes([ZCL_LEVEL_MOVE_UP, 0xFF]),
        )
        for _ in range(4):
            device.step_time(RAMP_PERIOD_MS + 10)
            pump(proc)
            for dpid, value in dp_writes(take_tx(proc)):
                if dpid == DP_LEVEL_L1:
                    # MCU still fading: reports where it is, behind the target.
                    mcu_report(proc, value_dp(DP_LEVEL_L1, max(TUYA_BRIGHTNESS_MIN, value - 40)))
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STOP_WITH_ON_OFF, None
        )
        assert level(device) >= 60 + 4 * RAMP_STEP - RAMP_STEP, level(device)
    finally:
        proc.stop()


def test_interim_fade_reports_do_not_move_current_level():
    proc, device = start()
    try:
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
            bytes([200, 0, 0]),
        )
        ack_writes(proc, device)
        mcu_report(proc, value_dp(DP_LEVEL_L1, 300))  # fading up
        mcu_report(proc, value_dp(DP_LEVEL_L1, 550))
        assert level(device) == 200
    finally:
        proc.stop()


def test_physical_dimming_after_holdoff_is_accepted():
    proc, device = start()
    try:
        set_level(proc, device, 200)
        mcu_report(proc, value_dp(DP_LEVEL_L1, 300))  # wall switch held, MCU dims
        assert level(device) == tuya_to_zcl(300)
    finally:
        proc.stop()


# --------------------------------------------------------------------------
# brightness range


def test_level_one_maps_inside_tuya_range():
    proc, device = start()
    try:
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
            bytes([1, 0, 0]),
        )
        levels = [v for dp, v in ack_writes(proc, device) if dp == DP_LEVEL_L1]
        assert levels and min(levels) >= TUYA_BRIGHTNESS_MIN
    finally:
        proc.stop()


def test_move_down_with_onoff_turns_off_at_bottom():
    proc, device = start()
    try:
        set_level(proc, device, 30)
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
            bytes([ZCL_LEVEL_MOVE_DOWN, 0xFF]),
        )
        writes = []
        for _ in range(10):
            device.step_time(RAMP_PERIOD_MS + 10)
            writes += ack_writes(proc, device)
        levels = [v for dp, v in writes if dp == DP_LEVEL_L1]
        assert all(v >= TUYA_BRIGHTNESS_MIN for v in levels), levels
        assert (DP_ON_L1, 0) in writes
        assert onoff(device) == 0
    finally:
        proc.stop()


def test_plain_move_down_stays_on_at_minimum():
    proc, device = start()
    try:
        set_level(proc, device, 30)
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE,
            bytes([ZCL_LEVEL_MOVE_DOWN, 0xFF, 0, 0]),
        )
        writes = []
        for _ in range(10):
            device.step_time(RAMP_PERIOD_MS + 10)
            writes += ack_writes(proc, device)
        assert (DP_ON_L1, 0) not in writes
        assert onoff(device) == 1
        assert level(device) >= 1
    finally:
        proc.stop()


# --------------------------------------------------------------------------
# commands without OnOff, and Step


def test_move_to_level_without_onoff():
    proc, device = start()
    try:
        set_level(proc, device, 50)
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL,
            bytes([128, 0, 0, 0, 0]),
        )
        writes = ack_writes(proc, device)
        assert (DP_LEVEL_L1, zcl_to_tuya(128)) in writes
        assert not [w for w in writes if w[0] == DP_ON_L1]
    finally:
        proc.stop()


def test_plain_move_and_stop_are_processed():
    proc, device = start()
    try:
        set_level(proc, device, 50)
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE,
            bytes([ZCL_LEVEL_MOVE_UP, 0xFF, 0, 0]),
        )
        for _ in range(3):
            device.step_time(RAMP_PERIOD_MS + 10)
            ack_writes(proc, device)
        device.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STOP, bytes([0, 0]))
        stopped = level(device)
        assert stopped > 50
        device.step_time(3 * RAMP_PERIOD_MS)
        ack_writes(proc, device)
        assert level(device) == stopped
    finally:
        proc.stop()


def test_step_changes_level_by_step_size():
    proc, device = start()
    try:
        set_level(proc, device, 100)
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STEP,
            bytes([ZCL_LEVEL_MOVE_UP, 50, 0, 0, 0, 0]),
        )
        writes = ack_writes(proc, device)
        assert (DP_LEVEL_L1, zcl_to_tuya(150)) in writes
        assert level(device) == 150
    finally:
        proc.stop()


def test_step_with_onoff_up_turns_on():
    proc, device = start()
    try:
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STEP_WITH_ON_OFF,
            bytes([ZCL_LEVEL_MOVE_UP, 40, 0, 0]),
        )
        writes = ack_writes(proc, device)
        assert (DP_ON_L1, 1) in writes
        assert onoff(device) == 1
    finally:
        proc.stop()


# --------------------------------------------------------------------------
# NV item ids must not move between releases


def _nv_ids() -> dict[str, list[int]]:
    """Evaluate the real macros with the C compiler."""
    root = Path(__file__).parent.parent
    prog = r"""
#include <stdio.h>
#include "device_config/nvm_items.h"
int main(void) {
    for (int i = 0; i < MAX_RELAYS; i++) printf("relay %d\n", NV_ITEM_RELAY_CLUSTER_DATA(i));
    for (int i = 0; i < MAX_COVER_SWITCHES; i++) printf("cover_switch %d\n", NV_ITEM_COVER_SWITCH_CONFIG(i));
    for (int i = 0; i < MAX_COVERS; i++) printf("cover %d\n", NV_ITEM_COVER_CONFIG(i));
    for (int i = 0; i < MAX_SWITCHES; i++) printf("switch %d\n", NV_ITEM_SWITCH_CLUSTER_DATA(i));
    printf("fixed %d\nfixed %d\nfixed %d\nfixed %d\nfixed %d\nfixed %d\nfixed %d\nfixed %d\n",
           NV_ITEM_CURRENT_VERSION_IN_NV, NV_ITEM_DEVICE_CONFIG, NV_ITEM_BASIC_CLUSTER_DATA,
           NV_ITEM_DEVICE_TYPE, NV_ITEM_MULTI_PRESS_RESET_COUNT, NV_ITEM_POLL_CONTROL_CONFIG,
           NV_ITEM_DP_CONFIG, NV_ITEM_DEVICE_CONFIG_EXT);
    return 0;
}
"""
    with tempfile.TemporaryDirectory() as tmp:
        c, exe = Path(tmp) / "nv.c", Path(tmp) / "nv"
        c.write_text(prog)
        subprocess.run(["gcc", "-I", str(root / "src"), str(c), "-o", str(exe)], check=True)
        out = subprocess.run([str(exe)], check=True, capture_output=True, text=True).stdout
    ids: dict[str, list[int]] = {}
    for line in out.splitlines():
        kind, value = line.split()
        ids.setdefault(kind, []).append(int(value))
    return ids


def test_nv_item_ids_match_released_layout():
    """Ids in 1.1.x: relays 9..13, cover switches 14..16, covers 17..19.
    Moving them makes an OTA-updated device read another item's data."""
    ids = _nv_ids()
    assert ids["relay"][:5] == [9, 10, 11, 12, 13]
    assert ids["cover_switch"] == [14, 15, 16]
    assert ids["cover"] == [17, 18, 19]


def test_nv_item_ids_are_unique():
    ids = _nv_ids()
    flat = [v for values in ids.values() for v in values]
    assert len(flat) == len(set(flat)), sorted(flat)


# --------------------------------------------------------------------------
# parser / endpoint layout


ZCL_CLUSTER_GROUPS = 0x0004
ZCL_CLUSTER_WINDOW_COVERING = 0x0102
ZCL_ATTR_COVER_FIRST = 0x0000  # WindowCoveringType


def test_dimmer_endpoints_have_groups():
    proc, device = start()
    try:
        for ep in (1, 2):
            assert device.read_zigbee_attr(ep, ZCL_CLUSTER_GROUPS, 0x0000) is not None
    finally:
        proc.stop()


def test_dimmer_count_is_capped_in_total():
    """Two DM tokens must not run past the four dimmer slots."""
    with StubProc(device_config="A;B;DM3;DM3;") as proc:
        device = Device(proc)
        for ep in (1, 2, 3, 4):
            assert device.read_zigbee_attr(ep, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_ATTR_CURRENT_LEVEL) is not None
        res = proc.exec(f"zcl_read 5 0x{ZCL_CLUSTER_LEVEL_CONTROL:04X} 0x0000")
        assert not res.ok


def test_cover_endpoint_comes_after_dimmers():
    """C<open><close> is a cover; it must get its own endpoint after the dimmer,
    not be laid over endpoint 1."""
    with StubProc(device_config="A;B;DM1;CC0C1;") as proc:
        device = Device(proc)
        assert device.read_zigbee_attr(1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_ATTR_CURRENT_LEVEL) is not None
        assert device.read_zigbee_attr(2, ZCL_CLUSTER_WINDOW_COVERING, ZCL_ATTR_COVER_FIRST) is not None
        res = proc.exec(f"zcl_read 1 0x{ZCL_CLUSTER_WINDOW_COVERING:04X} 0x{ZCL_ATTR_COVER_FIRST:04X}")
        assert not res.ok


# --------------------------------------------------------------------------
# default DP layout


def _dp_map(config: str) -> list[tuple[int, int]]:
    """Write on + level + min + max + switch type + power-on on both channels
    and return the DP writes, in order."""
    proc = StubProc(device_config=config).start()
    device = Device(proc)
    try:
        pump(proc)
        take_tx(proc)
        out = []
        for ep in (1, 2):
            device.call_zigbee_cmd(ep, ZCL_CLUSTER_LEVEL_CONTROL,
                                   ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF, bytes([100, 0, 0]))
            out += [dp for dp, _ in ack_writes(proc, device)]
            for cluster, attr, value in ((ZCL_CLUSTER_BALLAST, ZCL_ATTR_BALLAST_MIN, 20),
                                         (ZCL_CLUSTER_BALLAST, ZCL_ATTR_BALLAST_MAX, 200),
                                         (ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG, ZCL_ATTR_SWITCH_TYPE, 2)):
                device.write_zigbee_attr(ep, cluster, attr, value)
                out += [dp for dp, _ in ack_writes(proc, device)]
        device.write_zigbee_attr(1, ZCL_CLUSTER_ON_OFF, 0x4003, 0xFF)  # StartUpOnOff
        out += [dp for dp, _ in ack_writes(proc, device)]
        return out
    finally:
        proc.stop()


def test_default_dps_are_the_tuya_dimmer_layout():
    assert _dp_map(ZDMS16_2) == [1, 2, 3, 5, 4, 7, 8, 9, 11, 10, 14]


def test_explicit_p_and_u_tokens_match_the_defaults():
    assert _dp_map(ZDMS16_2_EXPLICIT) == _dp_map(ZDMS16_2)


def test_p_token_overrides_one_dimmer():
    cfg = "A;B;DM2;P01O21L22S23M24X25;U30;M;"
    assert _dp_map(cfg) == [1, 2, 3, 5, 4, 0x21, 0x22, 0x24, 0x25, 0x23, 0x30]


def test_module_button_reset_leaves_the_network():
    """The ZDMS16-2 has no keys on the MCU: the wall switch input dims and
    toggles, and only the module's own button makes the MCU ask for a leave.
    That has to work like the reset button of any other device. (Touch
    panels, whose MCU keys are the user interface, still refuse: see
    test_dp_attr.py.)"""
    proc, device = start()
    try:
        assert device.is_joined()
        proc.exec(f"uart_rx {tuya_frame(TUYA_CMD_RESET_NETWORK, bytes([0x01]))}")
        pump(proc)
        assert not device.is_joined(), "the module button must reset the device"
    finally:
        proc.stop()


# ---- OnLevel (Level Control 0x0011) ---------------------------------------

ZCL_ATTR_ON_LEVEL = 0x0011


def known_off(proc: StubProc) -> None:
    """The MCU has reported channel 1 off, as it does at boot."""
    mcu_report(proc, bool_dp(DP_ON_L1, 0))
    take_tx(proc)


def set_on_level(device: Device, lvl: int) -> None:
    device.write_zigbee_attr(1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_ATTR_ON_LEVEL, lvl)


def test_on_level_defaults_to_undefined():
    proc, device = start()
    try:
        assert int(device.read_zigbee_attr(1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_ATTR_ON_LEVEL)) == 0xFF
    finally:
        proc.stop()


def test_wall_switch_on_without_on_level_leaves_the_level_to_the_mcu():
    proc, device = start()
    try:
        known_off(proc)
        mcu_report(proc, bool_dp(DP_ON_L1, 1))  # wall switch
        assert ack_writes(proc, device) == []
        assert onoff(device) == 1
    finally:
        proc.stop()


def test_wall_switch_on_goes_straight_to_on_level():
    """The point of OnLevel here: no round trip through the network. The
    level is written as soon as the MCU reports the switch, and only the
    level (the MCU is already on)."""
    proc, device = start()
    try:
        dump_range(proc, 276, 591)
        set_on_level(device, 100)
        known_off(proc)
        mcu_report(proc, bool_dp(DP_ON_L1, 1))
        writes = ack_writes(proc, device)
        assert writes == [(DP_LEVEL_L1, scaled_to_tuya(100, 276, 591))]
        assert level(device) == 100
        assert onoff(device) == 1
    finally:
        proc.stop()


def test_first_report_after_boot_is_not_a_switch_press():
    """After a reboot of the Zigbee side the light may simply be on already;
    that must not be taken for someone switching it on."""
    proc, device = start()
    try:
        set_on_level(device, 100)
        mcu_report(proc, bool_dp(DP_ON_L1, 1))
        assert ack_writes(proc, device) == []
    finally:
        proc.stop()


def test_answer_to_our_query_is_not_a_switch_press():
    proc, device = start()
    try:
        set_on_level(device, 100)
        known_off(proc)
        mcu_report(proc, bool_dp(DP_ON_L1, 1), cmd=TUYA_CMD_REPORT_PASSIVE)
        assert ack_writes(proc, device) == []
    finally:
        proc.stop()


def test_own_level_command_is_not_overridden_by_on_level():
    """Our own on is echoed back by the MCU; that echo must not pull the
    light to OnLevel when the command asked for another level."""
    proc, device = start()
    try:
        dump_range(proc, 276, 591)
        set_on_level(device, 100)
        known_off(proc)
        device.call_zigbee_cmd(
            1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
            bytes([200, 0, 0]),
        )
        writes = ack_writes(proc, device)
        assert writes == [(DP_ON_L1, 1), (DP_LEVEL_L1, scaled_to_tuya(200, 276, 591))]
        assert level(device) == 200
    finally:
        proc.stop()


def test_on_command_uses_on_level():
    proc, device = start()
    try:
        dump_range(proc, 276, 591)
        set_on_level(device, 60)
        known_off(proc)
        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON)
        writes = ack_writes(proc, device)
        assert writes == [(DP_ON_L1, 1), (DP_LEVEL_L1, scaled_to_tuya(60, 276, 591))]
    finally:
        proc.stop()


def test_on_and_toggle_without_on_level_only_switch_on():
    proc, device = start()
    try:
        known_off(proc)
        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE)
        assert ack_writes(proc, device) == [(DP_ON_L1, 1)]
    finally:
        proc.stop()


TUYA_CMD_MCU_VERSION = 0x0B
TUYA_CMD_QUERY_ALL = 0x28
ATTR_MCU_VERSION = 0xFF12


def test_mcu_firmware_version_is_asked_and_shown():
    """The module asks for the MCU's firmware version at first contact and
    shows the answer (one byte, 2/2/4 bits) as a Basic cluster attribute."""
    proc, device = start()
    try:
        assert device.read_zigbee_attr(1, 0x0000, ATTR_MCU_VERSION) == ""
        device.step_time(2100)
        pump(proc)
        assert [cmd for cmd, _ in parse_frames(take_tx(proc))] == [TUYA_CMD_MCU_VERSION]
        proc.exec(f"uart_rx {tuya_frame(TUYA_CMD_MCU_VERSION, bytes([0x47]))}")
        pump(proc)
        assert device.read_zigbee_attr(1, 0x0000, ATTR_MCU_VERSION) == "1.0.7"
        # The answer frees the link at once: the query-all follows without
        # waiting for the ack timeout.
        assert TUYA_CMD_QUERY_ALL in [cmd for cmd, _ in parse_frames(take_tx(proc))]
        proc.exec(f"uart_rx {tuya_frame(TUYA_CMD_MCU_VERSION, bytes([0xFF]))}")
        pump(proc)
        assert device.read_zigbee_attr(1, 0x0000, ATTR_MCU_VERSION) == "3.3.15"
    finally:
        proc.stop()
