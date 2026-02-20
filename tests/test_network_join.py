from tests.client import StubProc
from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_ATTR_BASIC_MULTI_PRESS_RESET_COUNT,
    ZCL_CLUSTER_BASIC,
)

HAL_ZIGBEE_NETWORK_NOT_JOINED = 0
HAL_ZIGBEE_NETWORK_JOINED = 1
HAL_ZIGBEE_NETWORK_JOINING = 2


def test_tries_to_join_on_startup_not_joined() -> None:
    with StubProc(device_config="A;B;LB0;", joined=False) as proc:
        device = Device(proc)

        data = device.status()
        assert data["joined"] == str(HAL_ZIGBEE_NETWORK_JOINING)


def test_led_is_blinking() -> None:
    with StubProc(device_config="A;B;LB0;", joined=False) as proc:
        device = Device(proc)

        state = device.get_gpio("B0", refresh=True)
        for _ in range(4):
            device.step_time(500)
            assert device.get_gpio("B0", refresh=True) != state
            state = not state


def test_led_stops_blinking_after_join() -> None:
    with StubProc(device_config="A;B;LB0;", joined=False) as proc:
        device = Device(proc)

        device.set_network(HAL_ZIGBEE_NETWORK_JOINED)

        state = device.get_gpio("B0", refresh=True)
        for _ in range(4):
            device.step_time(500)
            assert device.get_gpio("B0", refresh=True) == state


def test_auto_joining_after_kicked() -> None:
    with StubProc(device_config="A;B;LB0;") as proc:
        device = Device(proc)

        device.set_network(HAL_ZIGBEE_NETWORK_NOT_JOINED)

        data = device.status()
        assert data["joined"] == str(HAL_ZIGBEE_NETWORK_JOINING)


def test_led_blinks_after_kicked() -> None:
    with StubProc(device_config="A;B;LB0;") as proc:
        device = Device(proc)

        device.set_network(HAL_ZIGBEE_NETWORK_NOT_JOINED)

        state = device.get_gpio("B0", refresh=True)
        for _ in range(4):
            device.step_time(500)
            assert device.get_gpio("B0", refresh=True) != state
            state = not state


def test_leaves_on_multipress() -> None:
    with StubProc(device_config="A;B;SA0u;RB1;") as proc:
        device = Device(proc)
        assert device.status()["joined"] == str(HAL_ZIGBEE_NETWORK_JOINED)

        # 9 presses should not cause the device to leave the network
        for _ in range(9):
            device.click_button("A0")

        assert device.status()["joined"] == str(HAL_ZIGBEE_NETWORK_JOINED)

        # 10th press should cause the device to leave the network
        device.click_button("A0")
        assert device.status()["joined"] != str(HAL_ZIGBEE_NETWORK_JOINED)


def test_leaves_on_multipress_configurable() -> None:
    with StubProc(device_config="A;B;SA0u;RB1;") as proc:
        device = Device(proc)
        assert device.status()["joined"] == str(HAL_ZIGBEE_NETWORK_JOINED)

        # Set multi-press count to reset to 5 via Basic cluster
        device.write_zigbee_attr(
            1,
            ZCL_CLUSTER_BASIC,
            ZCL_ATTR_BASIC_MULTI_PRESS_RESET_COUNT,
            5,
        )

        # 4 presses should not cause the device to leave the network
        for _ in range(4):
            device.click_button("A0")

        assert device.status()["joined"] == str(HAL_ZIGBEE_NETWORK_JOINED)

        # 5th press should cause the device to leave the network
        device.click_button("A0")
        assert device.status()["joined"] != str(HAL_ZIGBEE_NETWORK_JOINED)


def test_multipress_reset_disabled_when_zero() -> None:
    with StubProc(device_config="A;B;SA0u;RB1;") as proc:
        device = Device(proc)
        assert device.status()["joined"] == str(HAL_ZIGBEE_NETWORK_JOINED)

        # Disable multi-press reset by setting to 0 via Basic cluster
        device.write_zigbee_attr(
            1,
            ZCL_CLUSTER_BASIC,
            ZCL_ATTR_BASIC_MULTI_PRESS_RESET_COUNT,
            0,
        )

        # Pressing up to 15 times should never cause the device to leave
        for i in range(1, 16):
            device.click_button("A0")
            assert device.status()["joined"] == str(
                HAL_ZIGBEE_NETWORK_JOINED
            ), f"Device unexpectedly reset after {i} presses"


def test_leaves_on_onboard_button_long_press() -> None:
    with StubProc(device_config="A;B;BA0u;") as proc:
        device = Device(proc)
        assert device.status()["joined"] == str(HAL_ZIGBEE_NETWORK_JOINED)

        device.press_button("A0")
        device.step_time(3000)

        assert device.status()["joined"] != str(HAL_ZIGBEE_NETWORK_JOINING)


def test_announces_after_join() -> None:
    with StubProc(device_config="A;B;LB0;", joined=False) as proc:
        device = Device(proc)

        device.set_network(HAL_ZIGBEE_NETWORK_JOINED)

        device.wait_for_announce()
