"""relay_cluster_report(): the relay's own bindings must hear about a state
change no matter what caused it, not just a physical key press.

Before this, relay_cluster_on_relay_change() only notified the coordinator
(hal_zigbee_notify_attribute_changed) and persisted to NVM. A remote ZCL
command, a z2m/HA-initiated change, or an automation never reached anything
bound to the relay's own OnOff cluster -- only a switch endpoint's own
binding_action_on/off (fired from a button press) did. relay_cluster_report()
was declared in relay_cluster.h and never implemented; this closes that gap.

It always pushes an explicit ON or OFF, never TOGGLE, because a toggle
command has no state for the far end to converge to. And since
relay_on()/relay_off() fire the on_change callback unconditionally -- even
when the relay was already in that state -- the report is only resent when
it actually differs from what this cluster last pushed: without that, two
relays bound to each other would echo ON/OFF forever.
"""
from client import StubProc
from conftest import Device, RelayButtonPair
from zcl_consts import (
    ZCL_CLUSTER_ON_OFF,
    ZCL_CMD_ONOFF_OFF,
    ZCL_CMD_ONOFF_ON,
    ZCL_CMD_ONOFF_TOGGLE,
)


def test_remote_command_reports_to_bindings(
    device: Device, relay_button_pair: RelayButtonPair
):
    """A remote ZCL On command (z2m/HA/automation) must push to the relay's
    own bindings, not just a physical key press."""
    assert device.zcl_relay_get(relay_button_pair.relay_endpoint) == "0"

    device.zcl_relay_on(relay_button_pair.relay_endpoint)

    device.wait_for_cmd_send(
        relay_button_pair.relay_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )


def test_remote_off_command_reports_to_bindings(
    device: Device, relay_button_pair: RelayButtonPair
):
    device.zcl_relay_on(relay_button_pair.relay_endpoint)
    device.clear_events()

    device.zcl_relay_off(relay_button_pair.relay_endpoint)

    device.wait_for_cmd_send(
        relay_button_pair.relay_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )


def test_toggle_command_reports_explicit_state_not_toggle(
    device: Device, relay_button_pair: RelayButtonPair
):
    """The report must always be an explicit ON/OFF -- a TOGGLE command has
    no idea what state the far end should converge to."""
    device.call_zigbee_cmd(
        relay_button_pair.relay_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE
    )

    event = device.wait_for_cmd_send(relay_button_pair.relay_endpoint, ZCL_CLUSTER_ON_OFF)
    assert event.cmd in (ZCL_CMD_ONOFF_ON, ZCL_CMD_ONOFF_OFF)


def test_repeated_same_state_does_not_reemit(
    device: Device, relay_button_pair: RelayButtonPair
):
    """relay_on()/relay_off() fire on_change even when already in that
    state. Reporting again every time would mean two relays bound to each
    other never stop echoing."""
    device.zcl_relay_on(relay_button_pair.relay_endpoint)
    device.wait_for_cmd_send(
        relay_button_pair.relay_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )
    device.clear_events()

    # Same command again: relay was already on, on_change still fires.
    device.zcl_relay_on(relay_button_pair.relay_endpoint)

    assert device.zcl_list_cmds(
        endpoint=relay_button_pair.relay_endpoint, cluster=ZCL_CLUSTER_ON_OFF
    ) == []


def test_echo_of_own_report_breaks_the_loop(
    device: Device, relay_button_pair: RelayButtonPair
):
    """Simulates the round trip of two relays bound to each other: this
    relay pushes ON, the peer applies it and echoes ON back. The echo must
    not cause a second push, or the pair would loop forever."""
    device.zcl_relay_on(relay_button_pair.relay_endpoint)
    device.wait_for_cmd_send(
        relay_button_pair.relay_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_ON
    )
    device.clear_events()

    # The bound peer, having applied ON itself, echoes ON back to us.
    device.zcl_relay_on(relay_button_pair.relay_endpoint)

    assert device.zcl_list_cmds(
        endpoint=relay_button_pair.relay_endpoint, cluster=ZCL_CLUSTER_ON_OFF
    ) == []

    # A genuine state change afterwards must still get through.
    device.zcl_relay_off(relay_button_pair.relay_endpoint)
    device.wait_for_cmd_send(
        relay_button_pair.relay_endpoint, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_OFF
    )


def test_not_joined_does_not_report(device_config: str):
    with StubProc(device_config=device_config, joined=False) as proc:
        device = Device(proc)
        relay_endpoint = 5  # 4 switches (SA0u..SA3u) then 4 relays (RB0..RB3)

        device.zcl_relay_on(relay_endpoint)

        assert device.zcl_list_cmds(
            endpoint=relay_endpoint, cluster=ZCL_CLUSTER_ON_OFF
        ) == []
