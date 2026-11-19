"""Regression tests for the DP-backed roles (RT / ST / IT) and the UART tokens.

These exist because the single-letter dispatch in parse_config() silently
swallowed 'ST' (as S + pin "T0") and 'IT' (as I + pin "T1"), producing a
device that looked correct but wired nothing.
"""
from client import StubProc
from conftest import Device
from zcl_consts import (
    ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE,
    ZCL_ATTR_ONOFF,
    ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE,
    ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
    ZCL_CLUSTER_ON_OFF,
    ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
)

TPZ2 = "hktk6hze;TS0601-TPZ2;Y9600;ST18;ST19;RT18;RT19;IT25401;M;"


def test_st_creates_switch_endpoints_not_gpio_switches():
    """ST<hh> must build real switch endpoints with the multistate cluster."""
    proc = StubProc(device_config=TPZ2).start()
    try:
        device = Device(proc)
        for ep in (1, 2):
            assert device.read_zigbee_attr(
                ep, ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG,
                ZCL_ATTR_ONOFF_CONFIGURATION_SWITCH_MODE) is not None
            # This is the attribute z2m configures reporting on; if ST was
            # swallowed by the 'S' branch this is what breaks.
            assert device.read_zigbee_attr(
                ep, ZCL_CLUSTER_MULTISTATE_INPUT_BASIC,
                ZCL_ATTR_MULTISTATE_INPUT_PRESENT_VALUE) is not None
    finally:
        proc.stop()


def test_rt_creates_relay_endpoints():
    """RT<hh> relays must land on endpoints after the switches."""
    proc = StubProc(device_config=TPZ2).start()
    try:
        device = Device(proc)
        for ep in (3, 4):
            assert device.read_zigbee_attr(
                ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) is not None
    finally:
        proc.stop()


def test_dp_roles_claim_no_gpio():
    """DP-backed relays must not drive any pin: a GPIO build would toggle one."""
    proc = StubProc(device_config=TPZ2).start()
    try:
        device = Device(proc)
        before = {p: device.get_gpio(p, refresh=True)
                  for p in ("A0", "B4", "B5", "C0", "C2", "C3", "D2", "D7")}
        device.zcl_relay_on(3)
        device.zcl_relay_on(4)
        after = {p: device.get_gpio(p, refresh=True) for p in before}
        assert before == after, f"DP relay drove a GPIO: {before} -> {after}"
    finally:
        proc.stop()


def test_single_letter_tokens_do_not_swallow_two_letter_ones():
    """A config using only the two-letter roles must still build 4 endpoints."""
    proc = StubProc(device_config=TPZ2).start()
    try:
        device = Device(proc)
        assert device.read_zigbee_attr(
             4, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) is not None, \
            "endpoint 4 missing: ST/RT were probably parsed as S/R"
    finally:
        proc.stop()
