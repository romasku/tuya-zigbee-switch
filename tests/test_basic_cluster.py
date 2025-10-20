import datetime
from dataclasses import dataclass

import pytest

from tests.client import StubProc
from tests.conftest import Device, wait_for
from tests.zcl_consts import (
    ZCL_ATTR_BASIC_APP_VER,
    ZCL_ATTR_BASIC_DATE_CODE,
    ZCL_ATTR_BASIC_DEVICE_CONFIG,
    ZCL_ATTR_BASIC_HW_VER,
    ZCL_ATTR_BASIC_MFR_NAME,
    ZCL_ATTR_BASIC_MODEL_ID,
    ZCL_ATTR_BASIC_POWER_SOURCE,
    ZCL_ATTR_BASIC_STACK_VER,
    ZCL_ATTR_BASIC_STATUS_LED_STATE,
    ZCL_ATTR_BASIC_SW_BUILD_ID,
    ZCL_ATTR_BASIC_ZCL_VER,
    ZCL_CLUSTER_BASIC,
)


@dataclass
class AttrValueTestCase:
    name: str
    cluster: int
    attr: int
    expected: str


BASIC_ATTR_TEST_CASES = [
    AttrValueTestCase(
        name="ZCL Version",
        cluster=ZCL_CLUSTER_BASIC,
        attr=ZCL_ATTR_BASIC_ZCL_VER,
        expected="3",
    ),
    AttrValueTestCase(
        name="Application Version",
        cluster=ZCL_CLUSTER_BASIC,
        attr=ZCL_ATTR_BASIC_APP_VER,
        expected="3",
    ),
    AttrValueTestCase(
        name="Stack Version",
        cluster=ZCL_CLUSTER_BASIC,
        attr=ZCL_ATTR_BASIC_STACK_VER,
        expected="2",
    ),
    AttrValueTestCase(
        name="Hardware Version",
        cluster=ZCL_CLUSTER_BASIC,
        attr=ZCL_ATTR_BASIC_HW_VER,
        expected="0",
    ),
    AttrValueTestCase(
        name="Power Source",
        cluster=ZCL_CLUSTER_BASIC,
        attr=ZCL_ATTR_BASIC_POWER_SOURCE,
        expected="1",
    ),
]


@pytest.mark.parametrize("case", BASIC_ATTR_TEST_CASES, ids=lambda case: case.name)
def test_basic_cluster_read_attrs(device: Device, case: AttrValueTestCase):
    assert device.read_zigbee_attr(1, case.cluster, case.attr) == case.expected


def test_basic_cluster_read_manufacturer_and_model(
    device: Device,
    device_config: str,
):
    exp_manu, exp_model = device_config.split(";")[:2]

    assert (
        device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME)
        == exp_manu
    )
    assert (
        device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MODEL_ID)
        == exp_model
    )
    assert (
        device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_DEVICE_CONFIG)
        == device_config
    )


def test_basic_cluster_read_date_code(device: Device):
    date_code = device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_DATE_CODE)
    assert datetime.datetime.strptime(date_code, "%Y%m%d")  # dateCode


def test_basic_cluster_read_sw_build_id(device: Device):
    assert device.read_zigbee_attr(
        1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_SW_BUILD_ID
    ).startswith("1.0")


def test_network_led_control(device: Device) -> None:
    with StubProc(device_config="A;B;LB0;") as proc:
        device = Device(proc)
        device.write_zigbee_attr(
            1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_STATUS_LED_STATE, "1"
        )
        wait_for(lambda: device.get_gpio("B0", refresh=True), 1.0)

        device.write_zigbee_attr(
            1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_STATUS_LED_STATE, "0"
        )
        wait_for(lambda: not device.get_gpio("B0", refresh=True), 1.0)


@pytest.mark.parametrize("state", ["0", "1"], ids=lambda s: f"led_state={s}")
def test_network_led_preserved_via_nvm_control(device: Device, state: str) -> None:
    with StubProc(device_config="A;B;LB0;") as proc:
        device = Device(proc)
        device.write_zigbee_attr(
            1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_STATUS_LED_STATE, state
        )

    with StubProc(device_config="A;B;LB0;") as proc:
        device = Device(proc)
        assert (
            device.read_zigbee_attr(
                1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_STATUS_LED_STATE
            )
            == state
        )
        assert device.get_gpio("B0", refresh=True) == (state == "1")


def test_device_config_write_schedules_reset(tmp_path):
    # simple config will be written to NV and cause reset
    with StubProc(device_config="A;B;SA0u;RB0;") as proc:
        device = Device(proc)
        device.write_zigbee_attr(
            1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_DEVICE_CONFIG, "C;D;"
        )
        device.step_time(300)  # Device reboots after small delay
        assert proc.wait_for_exit(1.0)

    with StubProc() as proc:
        device = Device(proc)
        assert (
            device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_DEVICE_CONFIG)
            == "C;D;"
        )
        assert (
            device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME)
            == "C"
        )
        assert (
            device.read_zigbee_attr(1, ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MODEL_ID)
            == "D"
        )
