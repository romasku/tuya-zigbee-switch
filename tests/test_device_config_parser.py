import pytest

from client import StubProc
from conftest import Device
from zcl_consts import (
    ZCL_ATTR_BASIC_MFR_NAME,
    ZCL_ATTR_COVER_SWITCH_SWITCH_TYPE,
    ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
    ZCL_ATTR_ONOFF,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_ATTR_WINDOW_COVERING_MOVING,
    ZCL_CLUSTER_BASIC,
    ZCL_CLUSTER_COVER_SWITCH_CONFIG,
    ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
    ZCL_CLUSTER_WINDOW_COVERING,
)


def test_endpoints_layout_matches_config(device: Device, device_config: str):
    # smoke: basic cluster present on ep1
    # relying on zcl_list_attrs output via read interface
    # Count switch endpoints by switches in config
    parts = [p for p in device_config.split(";") if p]
    num_switches = sum(1 for p in parts[2:] if p.startswith("S"))
    num_relays = sum(1 for p in parts[2:] if p.startswith("R"))
    num_cover_switches = sum(1 for p in parts[2:] if p.startswith("X"))
    num_covers = sum(1 for p in parts[2:] if p.startswith("C"))

    # For each switch endpoint, check presence of clusters
    for ep in range(1, num_switches + 1):
        # Switch config cluster has attributes
        assert (
            device.read_zigbee_attr(
                ep,
                ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
                ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
            )
            is not None
        )
        # Multistate input has present value
        assert device.read_zigbee_attr(
            ep,
            ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
            ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
        ) in ("0", "1", "2")

    # For each relay endpoint check ONOFF present
    relay_start = num_switches + 1
    for ep in range(relay_start, relay_start + num_relays):
        assert device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) in (
            "0",
            "1",
        )

    # For each cover switch endpoint, check presence of clusters
    cover_switch_start = relay_start + num_relays
    for ep in range(cover_switch_start, cover_switch_start + num_cover_switches):
        # Cover switch config cluster has attributes
        assert (
            device.read_zigbee_attr(
                ep,
                ZCL_CLUSTER_COVER_SWITCH_CONFIG,
                ZCL_ATTR_COVER_SWITCH_SWITCH_TYPE,
            )
            is not None
        )
        # Multistate input has present value
        assert device.read_zigbee_attr(
            ep,
            ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
            ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
        ) in ("0", "1", "2")

    # For each cover endpoint check moving attribute present
    cover_start = cover_switch_start + num_cover_switches
    for ep in range(cover_start, cover_start + num_covers):
        assert device.read_zigbee_attr(
            ep,
            ZCL_CLUSTER_WINDOW_COVERING,
            ZCL_ATTR_WINDOW_COVERING_MOVING,
        ) in ("0", "1", "2")

    # A 1-Wire bus keeps its sensor endpoints even when the boot scan finds nothing.


def test_onewire_endpoints_report_unavailable_without_a_sensor():
    cfg = "ya5d6wth;TS000F-TH4;BB4u;LB7i;SD4u;RC2;SC0u;RC3;SA0u;RD7;SC4u;RD2;M;WB5;"
    proc = StubProc(device_config=cfg).start()
    try:
        device = Device(proc)
        for ep in range(9, 17):
            # 0x8000, the ZCL invalid temperature. The reply parser splits on
            # spaces, so the two little-endian bytes arrive as separate tokens.
            temp = proc.exec(f"zcl_read {ep} 0x0402 0x0000")
            assert temp.ok
            assert temp.payload["value"] == "00"
            assert temp.payload["80"] == ""
            assert device.read_zigbee_attr(ep, 0x0405, 0x0000) == "65535"
            assert device.read_zigbee_attr(ep, 0x0402, 0xFF00) == ""
        assert device.read_zigbee_attr(8, 0x0006, ZCL_ATTR_ONOFF) in ("0", "1")
        assert device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME)
    finally:
        proc.stop()


@pytest.mark.parametrize(
    "cfg",
    [
        "Manu;Model;",  # minimal
        "X;Y;SA0u;LB1;RB2;",  # dedicated status LED + one switch/relay
        "X;Y;SA0u;IB0;RB1;",  # indicator LED for relays
        "X;Y;XA0A1u;CB0B1;",  # cover switch + cover
        "X;Y;WB5;",  # 1-Wire bus only; sensor endpoints exist and read as N/A
    ],
)
def test_various_configs_boot(cfg: str):
    p = StubProc(device_config=cfg).start()
    try:
        d = Device(p)
        _ = d.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME)
    finally:
        p.stop()
