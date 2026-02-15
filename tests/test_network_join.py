import pytest

from tests.client import StubProc
from tests.conftest import Device

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


@pytest.mark.parametrize(
    "device_config,button",
    [
        ("A;B;SA0u;RB1;", "A0"),
        ("A;B;XA0A1u;CB0B1;", "A0"),
        ("A;B;XA0A1u;CB0B1;", "A1"),
    ],
    ids=["switch", "cover_switch_open", "cover_switch_close"],
)
def test_leaves_on_multipress(device_config: str, button: str) -> None:
    with StubProc(device_config=device_config) as proc:
        device = Device(proc)
        assert device.status()["joined"] == str(HAL_ZIGBEE_NETWORK_JOINED)

        # 9 presses should not cause the device to leave the network
        for _ in range(9):
            device.click_button(button)

        assert device.status()["joined"] == str(HAL_ZIGBEE_NETWORK_JOINED)

        # 10th press should cause the device to leave the network
        device.click_button(button)
        assert device.status()["joined"] != str(HAL_ZIGBEE_NETWORK_JOINED)


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
