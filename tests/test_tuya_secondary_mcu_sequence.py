"""Frames the module starts carry a sequence number that counts up by one."""
from client import StubProc
from conftest import Device
from test_dimmer_mcu import ZDMS16_2
from tuya_mcu import mcu_report, pump, take_tx

ZCL_CLUSTER_ON_OFF = 0x0006


def _seqs(raw: bytes) -> list[int]:
    out, i = [], 0
    while i + 9 <= len(raw):
        dlen = int.from_bytes(raw[i + 6:i + 8], "big")
        out.append(int.from_bytes(raw[i + 3:i + 5], "big"))
        i += 9 + dlen
    return out


def test_sequence_counts_up_by_one_per_frame():
    with StubProc(device_config=ZDMS16_2) as proc:
        device = Device(proc)
        pump(proc)
        take_tx(proc)
        seqs = []
        for i in range(6):
            device.call_zigbee_cmd(1, ZCL_CLUSTER_ON_OFF, 0x01 if i % 2 == 0 else 0x00)
            pump(proc)
            tx = take_tx(proc)
            seqs += _seqs(tx)
            mcu_report(proc, tx[8:-1])  # echo the DP unit back as the MCU does
        assert len(seqs) == 6
        assert all(b - a == 1 for a, b in zip(seqs, seqs[1:])), seqs
