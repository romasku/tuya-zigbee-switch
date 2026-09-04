from tests.client import StubProc
from tests.conftest import Device
from tests.zcl_consts import (
    ZCL_CLUSTER_LEVEL_CONTROL,
    ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
    ZCL_CMD_LEVEL_STOP_WITH_ON_OFF,
)

DIMMER_CFG = "A;B;DM2;P00O01L02S04M03X05;P01O07L08S0AM09X0B;U0E;M;"

# Ramp: step 12 ZCL units every 300ms (matches hardware dimmer).
RAMP_PERIOD_MS = 300
RAMP_STEP = 12


def test_move_up_ramps_by_step_and_stop_freezes():
    with StubProc(device_config=DIMMER_CFG) as proc:
        dev = Device(proc)
        dev.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF,
                            bytes([0x00, 0x0A]))
        for _ in range(5):
            dev.step_time(RAMP_PERIOD_MS + 10)
        lvl1 = int(dev.read_zigbee_attr(1, ZCL_CLUSTER_LEVEL_CONTROL, 0x0000))
        expected = 5 * RAMP_STEP
        expect = expected if expected <= 254 else 254
        assert lvl1 == expect, f"expected ~{expect} after 5 steps, got {lvl1}"

        dev.call_zigbee_cmd(1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STOP_WITH_ON_OFF)
        for _ in range(3):
            dev.step_time(RAMP_PERIOD_MS + 10)
        lvl2 = int(dev.read_zigbee_attr(1, ZCL_CLUSTER_LEVEL_CONTROL, 0x0000))
        assert lvl1 == lvl2, f"level moved after stop: {lvl1} -> {lvl2}"
