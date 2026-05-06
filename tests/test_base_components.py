from client import StubProc

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

def test_custom_debounce_delay_is_applied_to_switches():
    proc = StubProc(device_config="X;Y;D100;SA0u;RB0;").start()
    try:
        device = Device(proc)
        device.freeze_time()
        initial_relay_state = device.get_gpio("B0", refresh=True)

        device.set_gpio("A0", 0)
        device.step_time(99)
        assert device.get_gpio("B0", refresh=True) is initial_relay_state

        device.step_time(1)
        wait_for(lambda: device.get_gpio("B0", refresh=True) is not initial_relay_state)
    finally:
        proc.stop()
