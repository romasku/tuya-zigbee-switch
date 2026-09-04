"""The clock the coordinator writes must reach the secondary MCU.

We answered the MCU's time query with zeros -- epoch 1970 -- on an unchecked
assumption that it only mattered for countdowns. That is the one difference
that lines up with touch zones drifting on zigbee2mqtt and never on the Tuya
gateway, which supplies real time. These check the value actually crosses the
UART, and that an unset clock still reports unset rather than claiming 1970.
"""
from client import StubProc
from conftest import Device

TPZ2 = "hktk6hze;TS0601-TPZ2;Y9600;ST18;ST19;RT18;RT19;M;"
ZCL_CLUSTER_TIME = 0x000A
ZCL_ATTR_TIME = 0x0000
TUYA_CMD_SYNC_TIME = 0x24
ZIGBEE_EPOCH_OFFSET = 946684800

# An arbitrary but recognisable ZCL time: seconds since 2000-01-01.
ZCL_NOW = 800000000
EXPECTED_UNIX = ZCL_NOW + ZIGBEE_EPOCH_OFFSET


def tuya_frame(cmd: int, data: bytes = b"", seq: int = 1) -> str:
    body = (
        bytes([0x55, 0xAA, 0x02])
        + seq.to_bytes(2, "big")
        + bytes([cmd])
        + len(data).to_bytes(2, "big")
        + data
    )
    return (body + bytes([sum(body) % 256])).hex().upper()


def pump(proc: StubProc, times: int = 4) -> None:
    for _ in range(times):
        proc.exec("s")


def time_reply(tx: bytes):
    """Return the eight payload bytes of a 0x24 answer, or None."""
    i = 0
    while (i := tx.find(b"\x55\xaa", i)) != -1:
        if len(tx) >= i + 9 and tx[i + 5] == TUYA_CMD_SYNC_TIME:
            dlen = (tx[i + 6] << 8) | tx[i + 7]
            if dlen == 8 and len(tx) >= i + 8 + dlen:
                return tx[i + 8:i + 16]
        i += 2
    return None


def ask_for_time(proc: StubProc) -> bytes:
    proc.exec("uart_tx")  # drain
    proc.exec(f"uart_rx {tuya_frame(TUYA_CMD_SYNC_TIME)}")
    pump(proc)
    res = proc.exec("uart_tx")
    assert res.ok
    payload = time_reply(bytes.fromhex(res.payload["tx"]))
    assert payload is not None, "MCU never got an answer to its time query"
    return payload


def test_unset_clock_reports_unset():
    """Better to say nothing than to claim it is 1970."""
    proc = StubProc(device_config=TPZ2).start()
    try:
        Device(proc)
        pump(proc)
        assert ask_for_time(proc) == bytes(8)
    finally:
        proc.stop()


def test_written_clock_reaches_the_mcu():
    proc = StubProc(device_config=TPZ2).start()
    try:
        device = Device(proc)
        pump(proc)
        device.write_zigbee_attr(1, ZCL_CLUSTER_TIME, ZCL_ATTR_TIME, ZCL_NOW)
        pump(proc)
        payload = ask_for_time(proc)
        utc = int.from_bytes(payload[:4], "big")
        local = int.from_bytes(payload[4:], "big")
        assert abs(utc - EXPECTED_UNIX) <= 2, (
            f"MCU got {utc}, expected about {EXPECTED_UNIX}"
        )
        assert local == utc, "local time should track UTC until a zone is given"
    finally:
        proc.stop()
