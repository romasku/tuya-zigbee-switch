import pytest

from tests.client import StubProc
from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_ATTR_BASIC_MFR_NAME,
    ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
    ZCL_ATTR_ONOFF,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_CLUSTER_BASIC,
    ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
)


def test_endpoints_layout_matches_config(device: Device, device_config: str):
    # smoke: basic cluster present on ep1
    # relying on zcl_list_attrs output via read interface
    # Count switch endpoints by switches in config
    parts = [p for p in device_config.split(";") if p]
    num_switches = sum(1 for p in parts[2:] if p.startswith("S"))
    num_relays = sum(1 for p in parts[2:] if p.startswith("R"))

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
    for ep in range(num_switches + 1, num_switches + num_relays + 1):
        assert device.read_zigbee_attr(ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) in (
            "0",
            "1",
        )


@pytest.mark.parametrize(
    "cfg",
    [
        "Manu;Model;",  # minimal
        "X;Y;SA0u;LB1;RB2;",  # dedicated status LED + one switch/relay
        "X;Y;SA0u;IB0;RB1;",  # indicator LED for relays
    ],
)
def test_various_configs_boot(cfg: str):
    p = StubProc(device_config=cfg).start()
    try:
        d = Device(p)
        _ = d.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME)
    finally:
        p.stop()
