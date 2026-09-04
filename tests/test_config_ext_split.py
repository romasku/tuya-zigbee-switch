"""device_config_ext (0xff16) must parse as if concatenated onto device_config.

A board with enough RT/ST/PT tokens can run its config string past the
~74-character ceiling a single ZCL write allows. device_config_ext carries
the overflow, appended onto device_config in RAM before parse_config()'s
token loop runs. These check that a config split across the two strings
produces exactly the same device that the single, unsplit string would --
see test_tuya_dp_roles.py for the single-string equivalent this mirrors --
and that manufacturer/model, which only device_config carries a header for,
still comes out right when the split lands mid-peripheral-list.
"""
from client import StubProc
from conftest import Device
from zcl_consts import (
    ZCL_ATTR_BASIC_MFR_NAME,
    ZCL_ATTR_BASIC_MODEL_ID,
    ZCL_ATTR_ONOFF,
    ZCL_CLUSTER_BASIC,
    ZCL_CLUSTER_ON_OFF,
)

# The real NovaDigital 6-gang entry (device_db.yaml), split mid-list: three
# RT tokens in the base string, the rest -- including the trailing PT/M
# tokens -- in the ext string.
BASE = "tdhnhhiy;TS0601-6G;WB1B7;Y9600;RT01;RT02;RT03;"
EXT = "RT04;RT05;RT06;PT0E;M;"


def test_ext_relays_create_relay_endpoints():
    """The 3 RT tokens carried in device_config_ext must build real relay
    endpoints (4-6), same as if they were in device_config directly."""
    proc = StubProc(device_config=BASE, device_config_ext=EXT).start()
    try:
        device = Device(proc)
        for ep in range(1, 7):
            assert device.read_zigbee_attr(
                ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) is not None
    finally:
        proc.stop()


def test_manufacturer_and_model_come_from_base_string_only():
    """device_config_ext has no manufacturer;model header -- it must not be
    mistaken for one, and the base string's header must still win."""
    proc = StubProc(device_config=BASE, device_config_ext=EXT).start()
    try:
        device = Device(proc)
        assert device.read_zigbee_attr(
            1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME) == "tdhnhhiy"
        assert device.read_zigbee_attr(
            1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MODEL_ID) == "TS0601-6G"
    finally:
        proc.stop()


def test_missing_ext_string_does_not_break_base_parsing():
    """No device_config_ext at all (the common case) must parse exactly as
    before -- an empty NV item must not be treated as a truncated token."""
    proc = StubProc(device_config=BASE + "RT04;RT05;RT06;PT0E;M;").start()
    try:
        device = Device(proc)
        for ep in range(1, 7):
            assert device.read_zigbee_attr(
                ep, ZCL_CLUSTER_ON_OFF, ZCL_ATTR_ONOFF) is not None
    finally:
        proc.stop()
