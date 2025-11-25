from tests.conftest import Device, wait_for


def test_button_debounce_stronger(device: Device, button_pin: str, relay_pin: str):
    # Extended debounce: chatter beyond DEBOUNCE should collapse to a single press
    device.freeze_time()
    # chatter for 200ms total around the edge
    for i in range(20):
        device.set_gpio(button_pin, 0 if i % 2 == 0 else 1)
        device.step_time(10)
    device.set_gpio(button_pin, 0)
    device.step_time(60)  # past debounce stable
    wait_for(lambda: device.get_gpio(relay_pin) is True)
