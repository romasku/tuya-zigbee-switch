"""Tuya secondary-MCU serial protocol helpers for the stub tests: build frames
the MCU would send, and decode what the firmware sent (stub uart_rx/uart_tx)."""
from client import StubProc

TUYA_CMD_NETWORK_STATUS = 0x02
TUYA_CMD_RESET_NETWORK = 0x03
TUYA_CMD_WRITE = 0x04
TUYA_CMD_REPORT_PASSIVE = 0x05
TUYA_CMD_REPORT = 0x06
TUYA_CMD_QUERY_NETWORK = 0x20
TUYA_CMD_SYNC_TIME = 0x24
TUYA_CMD_QUERY_ALL = 0x28

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


def value_dp(dp: int, value: int) -> bytes:
    return dp_payload(dp, TUYA_TYPE_VALUE, value.to_bytes(4, "big"))


def bool_dp(dp: int, value: int) -> bytes:
    return dp_payload(dp, TUYA_TYPE_BOOL, bytes([value]))


def enum_dp(dp: int, value: int) -> bytes:
    return dp_payload(dp, TUYA_TYPE_ENUM, bytes([value]))


def pump(proc: StubProc, times: int = 4) -> None:
    for _ in range(times):
        proc.exec("s")


def take_tx(proc: StubProc) -> bytes:
    res = proc.exec("uart_tx")
    assert res.ok
    return bytes.fromhex(res.payload["tx"])


def parse_frames(raw: bytes) -> list[tuple[int, bytes]]:
    """Split a TX capture into (cmd, data) frames, checking every checksum."""
    frames = []
    i = 0
    while i + 9 <= len(raw):
        assert raw[i : i + 3] == b"\x55\xaa\x02", raw[i:].hex()
        cmd = raw[i + 5]
        dlen = int.from_bytes(raw[i + 6 : i + 8], "big")
        end = i + 8 + dlen
        assert sum(raw[i:end]) % 256 == raw[end], "bad checksum"
        frames.append((cmd, raw[i + 8 : end]))
        i = end + 1
    assert i == len(raw), f"trailing bytes {raw[i:].hex()}"
    return frames


def dp_writes(raw: bytes) -> list[tuple[int, int]]:
    """(dpid, value) for every DP write in a TX capture."""
    out = []
    for cmd, data in parse_frames(raw):
        if cmd != TUYA_CMD_WRITE:
            continue
        dpid, _, vlen = data[0], data[1], int.from_bytes(data[2:4], "big")
        out.append((dpid, int.from_bytes(data[4 : 4 + vlen], "big")))
    return out


def mcu_report(proc: StubProc, *dps: bytes, cmd: int = TUYA_CMD_REPORT) -> None:
    proc.exec(f"uart_rx {tuya_frame(cmd, b''.join(dps))}")
    pump(proc)


def dp_units(raw: bytes) -> list[bytes]:
    """The DP units (dpid, type, len, value) of every DP write in a capture."""
    return [data for cmd, data in parse_frames(raw) if cmd == TUYA_CMD_WRITE]
