"""Runtime tests for the datapoint <-> ZCL attribute bridge.

The Tuya secondary MCU path had no runtime coverage: every existing test
asserted against the source text, so an endianness slip or a dropped command
byte would pass. These drive the real UART both ways through the stub.
"""
from client import StubProc
from conftest import Device

ZCL_CLUSTER_BASIC = 0x0000

TPZ2 = "hktk6hze;TS0601-TPZ2;Y9600;ST18;ST19;RT181E;RT191F;PT26;M;"
DP_CONFIG = "12E20;13E21;24B22;25E23;67B24;68E25;69V26;6AV27;"

ATTR_MODE_L1 = 0xFF20
ATTR_BACKLIGHT = 0xFF22
ATTR_INDICATOR = 0xFF23
ATTR_VIBRATION = 0xFF25
ATTR_MOMENTARY_1 = 0xFF26

TUYA_CMD_WRITE = 0x04
TUYA_CMD_REPORT_PASSIVE = 0x05
TUYA_CMD_REPORT = 0x06

TUYA_TYPE_BOOL = 0x01
TUYA_TYPE_VALUE = 0x02
TUYA_TYPE_ENUM = 0x04


def tuya_frame(cmd: int, data: bytes = b"", seq: int = 1) -> str:
    body = (
        bytes([0x55, 0xAA, 0x02])
        + seq.to_bytes(2, "big")
        + bytes([cmd])
        + len(data).to_bytes(2, "big")
        + data
    )
    return (body + bytes([sum(body) % 256])).hex().upper()


def dp_payload(dp: int, dp_type: int, value: bytes) -> bytes:
    return bytes([dp, dp_type]) + len(value).to_bytes(2, "big") + value


def pump(proc: StubProc, times: int = 4) -> None:
    """Give the main loop a chance to drain the UART."""
    for _ in range(times):
        proc.exec("s")


def take_tx(proc: StubProc) -> bytes:
    res = proc.exec("uart_tx")
    assert res.ok
    return bytes.fromhex(res.payload["tx"])


def start() -> StubProc:
    return StubProc(device_config=TPZ2, dp_config=DP_CONFIG).start()


def test_dp_config_registers_attributes():
    """Every entry in the dp config must show up on Basic of endpoint 1."""
    proc = start()
    try:
        device = Device(proc)
        for attr in (0xFF20, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27):
            assert device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, attr) is not None
    finally:
        proc.stop()


def test_dp_config_string_is_readable():
    """0xff11 carries the map, so it can be rewritten over the air."""
    proc = start()
    try:
        device = Device(proc)
        assert "12E20" in device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, 0xFF11)
    finally:
        proc.stop()


def test_write_bool_attribute_emits_tuya_write():
    """Writing the backlight attribute must push DP 0x24 as a bool."""
    proc = start()
    try:
        device = Device(proc)
        pump(proc)
        take_tx(proc)  # discard boot chatter
        device.write_zigbee_attr(1, ZCL_CLUSTER_BASIC, ATTR_BACKLIGHT, 1)
        pump(proc)
        tx = take_tx(proc)
        assert dp_payload(0x24, TUYA_TYPE_BOOL, bytes([1])) in tx
    finally:
        proc.stop()


def test_write_value_attribute_is_big_endian():
    """Tuya values are 4 byte big endian; ZCL stores them little endian."""
    proc = start()
    try:
        device = Device(proc)
        pump(proc)
        take_tx(proc)
        device.write_zigbee_attr(1, ZCL_CLUSTER_BASIC, ATTR_MOMENTARY_1, 300)
        pump(proc)
        tx = take_tx(proc)
        assert dp_payload(0x69, TUYA_TYPE_VALUE, (300).to_bytes(4, "big")) in tx
    finally:
        proc.stop()


def test_report_updates_attribute():
    """A datapoint report from the MCU must land on the attribute."""
    proc = start()
    try:
        device = Device(proc)
        frame = tuya_frame(
            TUYA_CMD_REPORT, dp_payload(0x68, TUYA_TYPE_ENUM, bytes([2]))
        )
        proc.exec(f"uart_rx {frame}")
        pump(proc)
        assert int(device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ATTR_VIBRATION)) == 2
    finally:
        proc.stop()


def test_passive_report_is_decoded():
    """0x05 is how the MCU answers a query; dropping it loses every value."""
    proc = start()
    try:
        device = Device(proc)
        frame = tuya_frame(
            TUYA_CMD_REPORT_PASSIVE, dp_payload(0x25, TUYA_TYPE_ENUM, bytes([2]))
        )
        proc.exec(f"uart_rx {frame}")
        pump(proc)
        assert int(device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ATTR_INDICATOR)) == 2
    finally:
        proc.stop()


def test_value_report_is_decoded_big_endian():
    proc = start()
    try:
        device = Device(proc)
        frame = tuya_frame(
            TUYA_CMD_REPORT,
            dp_payload(0x69, TUYA_TYPE_VALUE, (600).to_bytes(4, "big")),
        )
        proc.exec(f"uart_rx {frame}")
        pump(proc)
        value = int(device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ATTR_MOMENTARY_1))
        assert value == 600
    finally:
        proc.stop()


def test_relay_datapoints_still_reach_the_relay():
    """dp_attr sits in front of tuya_dp_relay in the callback chain: a
    datapoint it does not own has to be forwarded, not swallowed."""
    proc = StubProc(device_config=TPZ2, dp_config=DP_CONFIG).start()
    try:
        device = Device(proc)
        frame = tuya_frame(
            TUYA_CMD_REPORT, dp_payload(0x18, TUYA_TYPE_BOOL, bytes([1]))
        )
        proc.exec(f"uart_rx {frame}")
        pump(proc)
        assert int(device.read_zigbee_attr(3, 0x0006, 0x0000)) == 1
    finally:
        proc.stop()


def test_malformed_entry_does_not_drop_the_rest():
    """A bad token must cost only its own entry."""
    proc = StubProc(
        device_config=TPZ2, dp_config="24B22;ZZZZZ;68E25;"
    ).start()
    try:
        device = Device(proc)
        assert device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, 0xFF22) is not None
        assert device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, 0xFF25) is not None
    finally:
        proc.stop()


TUYA_CMD_CONFIGURE_MODULE = 0x03
TUYA_CMD_REPORT_NETWORK_STATUS = 0x02
ZCL_CLUSTER_MULTISTATE_INPUT = 0x0012
ZCL_ATTR_PRESENT_VALUE = 0x0055


def test_hold_does_not_leave_a_working_network():
    """The MCU asks us to leave when a key is held. An accidental long press is
    the most common way a switch drops off a production network, so a joined
    device must refuse and report itself connected instead."""
    proc = start()
    try:
        device = Device(proc)
        pump(proc)
        take_tx(proc)
        frame = tuya_frame(TUYA_CMD_CONFIGURE_MODULE, bytes([0x01]))
        proc.exec(f"uart_rx {frame}")
        pump(proc)
        tx = take_tx(proc)
        assert device.is_joined(), "hold must never drop a joined device"
        # 0x02 "network status" carrying 0x01 = connected.
        assert bytes([TUYA_CMD_REPORT_NETWORK_STATUS, 0x00, 0x01, 0x01]) in tx
    finally:
        proc.stop()


def _saw_press(device: Device) -> bool:
    """A key press shows up as a change on the multistate input of endpoint 1,
    which is the attribute z2m subscribes to for switch actions."""
    for e in device._events:
        if e.kind != "zcl_attr_change":
            continue
        if (
            int(e.payload["ep"]) == 1
            and int(e.payload["cluster"], 16) == ZCL_CLUSTER_MULTISTATE_INPUT
            and int(e.payload["attr"], 16) == ZCL_ATTR_PRESENT_VALUE
        ):
            return True
    return False


def test_passive_report_does_not_fake_a_key_press():
    """0x05 is the MCU acking something we sent. Treating it as a touch fires
    switch actions and feeds the multi-press factory reset."""
    proc = start()
    try:
        device = Device(proc)
        pump(proc)
        device.clear_events()
        frame = tuya_frame(
            TUYA_CMD_REPORT_PASSIVE, dp_payload(0x18, TUYA_TYPE_BOOL, bytes([1]))
        )
        proc.exec(f"uart_rx {frame}")
        pump(proc)
        assert not _saw_press(device), "passive report must not look like a press"
    finally:
        proc.stop()


def test_proactive_report_still_counts_as_a_key_press():
    """The suppression above must not deafen the real thing."""
    proc = start()
    try:
        device = Device(proc)
        pump(proc)
        device.clear_events()
        frame = tuya_frame(
            TUYA_CMD_REPORT, dp_payload(0x18, TUYA_TYPE_BOOL, bytes([1]))
        )
        proc.exec(f"uart_rx {frame}")
        pump(proc)
        assert _saw_press(device), "a real touch must still register"
    finally:
        proc.stop()


TPZ2_WITH_INIT = (
    "hktk6hze;TS0601-TPZ2;Y9600;ST18;ST19;RT181E;RT191F;IT13400;M;"
)
TUYA_CMD_QUERY_NETWORK = 0x20


def _wrote_dp(tx: bytes, dp: int) -> bool:
    """True if a 0x04 write for this datapoint appears in the TX stream."""
    i = 0
    while (i := tx.find(b"\x55\xaa", i)) != -1:
        if len(tx) >= i + 9 and tx[i + 5] == TUYA_CMD_WRITE and tx[i + 8] == dp:
            return True
        i += 2
    return False


def test_dp_inits_reach_the_mcu():
    """The IT-declared values must actually be written.

    Note this does NOT guard the timing fix that moved these out of app_init().
    The stub produces identical output either way -- verified by reverting the
    change and watching this still pass. The race only exists on real hardware,
    where writing the config string reboots the Telink while the MCU keeps
    running mid conversation and drops what it cannot resync to. That is why a
    settings change used to need a power cycle to take effect, and the fix
    rests on reading the code plus that observed symptom, not on this test.
    """
    proc = StubProc(device_config=TPZ2_WITH_INIT).start()
    try:
        Device(proc)
        pump(proc)
        take_tx(proc)
        proc.exec(f"uart_rx {tuya_frame(TUYA_CMD_QUERY_NETWORK)}")
        pump(proc)
        assert _wrote_dp(take_tx(proc), 0x13), "IT never reached the MCU"
    finally:
        proc.stop()
