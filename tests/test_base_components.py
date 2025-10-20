from tests.conftest import Device, wait_for


def test_relay_off_pin_mutual_exclusion():
    from tests.client import StubProc

    # Relay R B0 with off pin C1
    # Off-pin immediately follows the relay pin (no extra 'R' prefix)
    cfg = "X;Y;SA0u;RB0C1;"
    p = StubProc(device_config=cfg).start()
    try:
        d = Device(p)
        # Relay endpoint is 2 (1 switch)
        ep = 2
        d.call_zigbee_cmd(ep, 0x0006, 0x01)
        wait_for(lambda: d.get_gpio("B0") is True)
        assert d.get_gpio("C1") is False
        d.call_zigbee_cmd(ep, 0x0006, 0x00)
        wait_for(lambda: d.get_gpio("B0") is False)
        assert d.get_gpio("C1") is True
    finally:
        p.stop()


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
