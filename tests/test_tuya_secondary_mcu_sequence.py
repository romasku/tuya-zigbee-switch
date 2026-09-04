from pathlib import Path


def test_tuya_secondary_mcu_sequence_advances_by_1_per_frame():
    source = Path("src/zigbee/tuya_secondary_mcu.c").read_text()

    assert "g_tx_seq + 0x10" not in source
    assert "g_tx_seq + 1" in source
    assert "0xFFF0" in source


def test_tuya_secondary_mcu_sequence_wraps_at_fff0():
    source = Path("src/zigbee/tuya_secondary_mcu.c").read_text()

    assert "g_tx_seq >= 0xFFF0" in source
    assert "? 0 :" in source
