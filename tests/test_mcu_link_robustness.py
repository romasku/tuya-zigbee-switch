"""Robustness of the secondary-MCU link: framing, TX queue order, first
contact, relay echo window, and endpoints that share clusters."""
from client import StubProc
from conftest import Device
from test_dimmer_mcu import (
    DP_LEVEL_L1,
    DP_ON_L1,
    TUYA_CMD_REPORT,
    TUYA_TYPE_BOOL,
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CMD_LEVEL_MOVE_TO_LEVEL,
    ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF,
    ZDMS16_2,
    bool_dp,
    dp_payload,
    dp_writes,
    mcu_report,
    onoff,
    parse_frames,
    pump,
    set_level,
    start,
    take_tx,
    tuya_frame,
    value_dp,
)


# ---------------------------------------------------------------- RX framing

def test_dlen_fff7_does_not_hang_rx_loop():
    """frame_len = 8 + 0xFFF7 + 1 wraps to 0 in uint16_t: checksum reads 64 KiB
    past g_rx_assembly, index [-1], memmove of 0 bytes, `continue` forever."""
    proc, device = start()
    try:
        proc.exec("uart_rx 55AA0200000" + "6FFF7")
        res = proc.exec("s", timeout=2.0)   # main loop runs the RX assembly
        assert res.ok
    finally:
        proc.stop()


def test_truncated_frame_does_not_eat_a_later_report():
    """No inter-byte timeout: a partial frame stays in the assembly buffer and
    swallows the next real frame, however much later it comes."""
    proc, device = start()
    try:
        # a frame cut short (dropped DMA chunk / ring overflow / glitch)
        proc.exec("uart_rx " + tuya_frame(TUYA_CMD_REPORT, bool_dp(DP_ON_L1, 1))[:18])
        pump(proc)
        device.step_time(60_000)
        pump(proc)
        # a minute later the wall switch turns channel 1 on
        mcu_report(proc, bool_dp(DP_ON_L1, 1))
        assert onoff(device, 1) == 1, "real report lost behind a stale partial frame"
    finally:
        proc.stop()


# ---------------------------------------------------------------- TX queue

def test_coalescing_must_not_move_level_behind_off():
    """Off / on-at-80 / off while a frame is in flight. In order the MCU would
    see on, level 80, off (and remember 80). Coalescing the second off into the
    queued 'on' slot sends: off, level 80 -> a level written while the channel
    is off, which the ZDMS16-2 acks but does not keep."""
    proc, device = start()
    try:
        set_level(proc, device, 50)   # channel on
        # level write while on: the MCU does not answer for ~0.9 s -> in flight
        device.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_TO_LEVEL,
                               bytes([60, 0, 0, 0, 0]))
        pump(proc)
        assert [dp for dp, _ in dp_writes(take_tx(proc))] == [DP_LEVEL_L1]
        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, 0x00)                       # off
        device.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL,
                               ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF, bytes([80, 0, 0]))
        device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, 0x00)                       # off
        writes = []
        for _ in range(6):
            device.step_time(200)       # in-flight write times out
            pump(proc)
            w = dp_writes(take_tx(proc))
            for dpid, v in w:
                writes.append((dpid, v))
                mcu_report(proc, dp_payload(dpid, TUYA_TYPE_BOOL, bytes([v]))
                           if dpid == DP_ON_L1 else value_dp(dpid, v))
        # MCU state walk: is the last level write done while the channel is on?
        on = True
        level_kept_while_on = None
        for dpid, v in writes:
            if dpid == DP_ON_L1:
                on = bool(v)
            elif dpid == DP_LEVEL_L1:
                level_kept_while_on = on
        assert level_kept_while_on is not False, f"level written while off: {writes}"
    finally:
        proc.stop()


def test_query_all_respects_one_frame_in_flight():
    """IT init in flight, then 0x28 goes out at once (send_cmd bypasses the
    queue): two frames outstanding at first contact."""
    cfg = "hktk6hze;TS0601-TPZ2;Y9600;ST18;ST19;RT181E;RT191F;IT13400;IT12400;M;"
    proc = StubProc(device_config=cfg, dp_config="12E20;").start()
    try:
        Device(proc)
        pump(proc)
        take_tx(proc)
        proc.exec(f"uart_rx {tuya_frame(0x20)}")
        pump(proc)
        frames = [c for c, _ in parse_frames(take_tx(proc)) if c in (0x04, 0x28)]
        assert frames.count(0x04) + frames.count(0x28) <= 1, frames
    finally:
        proc.stop()


# ---------------------------------------------------------------- first contact

def test_it_inits_follow_the_mcus_first_network_query():
    """Base PR waited for the MCU's 0x20 (comment: before that the MCU 'drops
    whatever we send'). The 2 s timer now fires first and the later 0x20 no
    longer triggers the inits."""
    cfg = "hktk6hze;TS0601-TPZ2;Y9600;ST18;ST19;RT181E;RT191F;IT13400;M;"
    proc = StubProc(device_config=cfg).start()
    try:
        device = Device(proc)
        pump(proc)
        take_tx(proc)
        device.step_time(2500)     # MCU still booting / resyncing: nothing heard
        pump(proc)
        take_tx(proc)              # whatever went out here was not heard
        proc.exec(f"uart_rx {tuya_frame(0x20)}")
        pump(proc)
        tx = take_tx(proc)
        device.step_time(200)      # the timer's unanswered IT write times out first
        pump(proc)
        tx += take_tx(proc)
        assert any(dp == 0x13 for dp, _ in dp_writes(tx)), "IT not sent after the MCU's 0x20"
    finally:
        proc.stop()


# ---------------------------------------------------------------- echo window vs queue

TPZ2 = "hktk6hze;TS0601-TPZ2;Y9600;ST18;ST19;RT181E;RT191F;PT26;M;"
DP_CONFIG = "12E20;13E21;24B22;25E23;67B24;68E25;69V26;6AV27;"


def test_relay_echo_after_queue_delay_is_not_a_key_press():
    """Echo window (700 ms) is armed when the relay write is QUEUED. Five
    unanswered config writes ahead of it (5 x 150 ms) push the real write past
    the window; the MCU's immediate echo is then taken for a touch."""
    proc = StubProc(device_config=TPZ2, dp_config=DP_CONFIG).start()
    try:
        device = Device(proc)
        pump(proc)
        take_tx(proc)
        for attr, val in ((0xFF20, 1), (0xFF21, 1), (0xFF22, 1), (0xFF23, 1), (0xFF24, 1)):
            device.write_zigbee_attr(1, 0x0000, attr, val)
        device.zcl_relay_on(3)            # relay DP 0x18 (= key DP of switch ep 1)
        pump(proc)
        device.clear_events()
        echoed = False
        for _ in range(12):
            device.step_time(150)
            pump(proc)
            for dpid, v in dp_writes(take_tx(proc)):
                if dpid == 0x18:          # MCU echoes the relay at once
                    mcu_report(proc, bool_dp(0x18, v))
                    echoed = True
        assert echoed
        presses = [e for e in device._events if e.kind == "zcl_attr_change"
                   and int(e.payload["ep"]) == 1 and int(e.payload["cluster"], 16) == 0x0012]
        assert not presses, "relay echo became a synthetic key press"
    finally:
        proc.stop()


# ---------------------------------------------------------------- shared clusters

def test_switch_type_write_on_dimmer_endpoint():
    """genOnOffSwitchCfg lives on dimmer endpoints too; writing it went
    through the switch cluster path with a NULL cluster (crash)."""
    proc, device = start()
    try:
        device.write_zigbee_attr(1, 0x0007, 0x0000, 2)
        pump(proc)
        assert (0x04, 2) in dp_writes(take_tx(proc))  # DP 4 = switch type L1
        assert proc.exec("s").ok
    finally:
        proc.stop()


def test_no_time_cluster_and_time_sync_answered_with_zeros():
    """No genTime: nothing would ever write it (z2m does not push the time).
    The MCU's time request (0x24) still gets its reply, eight zero bytes."""
    proc, device = start()
    try:
        assert not proc.exec("zcl_read 1 0x000A 0x0000").ok
        proc.exec(f"uart_rx {tuya_frame(0x24, seq=0x1234)}")
        pump(proc)
        replies = [(cmd, data) for cmd, data in parse_frames(take_tx(proc)) if cmd == 0x24]
        assert replies == [(0x24, bytes(8))]
    finally:
        proc.stop()
