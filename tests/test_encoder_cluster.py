import pytest

from client import StubProc
from conftest import Device
from tests.zcl_consts import (
  ZCL_CLUSTER_ON_OFF,
  ZCL_CMD_ONOFF_TOGGLE,
  ZCL_CLUSTER_LEVEL_CONTROL,
  ZCL_CMD_LEVEL_STEP,
  ZCL_LEVEL_MOVE_UP,
  ZCL_CLUSTER_LIGHTING_COLOR_CONTROL,
  ZCL_CMD_LIGHTING_COLOR_STEP_TEMP
)

@pytest.fixture
def encoder_device() -> Device:
    p = StubProc(device_config="X;Y;EA0uA1uA2u;").start() 
    # EA0uA1uA2u;
    d = Device(p)
    yield d  

def test_encoder_cluster_switch_pressing_causes_on_off_command(encoder_device: Device) -> None:
  
    # Test setup
    encoder_device.step_time(100)
    result = encoder_device.set_gpio("A2", 0) # Press
    encoder_device.step_time(100)
    result = encoder_device.set_gpio("A2", 1) # Release
    encoder_device.step_time(100)

    # Assertions
    encoder_device.wait_for_cmd_send(
        1, ZCL_CLUSTER_ON_OFF, ZCL_CMD_ONOFF_TOGGLE
    )

def test_encoder_cluster_rotating_clockwise_causes_step_up_command(encoder_device: Device) -> None:
  
    # Test setup
    encoder_device.step_time(100)

    # When Rotating clockwise Pin A changes before Pin B
    result = encoder_device.set_gpio("A0", 0)
    encoder_device.step_time(100)
    result = encoder_device.set_gpio("A1", 0) 
    encoder_device.step_time(100)

    # Assertions
    cmd = encoder_device.wait_for_cmd_send(
        1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STEP
    )
    assert cmd.data == b'\x00\x0D\x01\x00' # x00 Move Up, StepSize x0D, Transistion Time \x01\x00 (1 x 1/10th Sec)

def test_encoder_cluster_rotating_anticlockwise_causes_step_down_command(encoder_device: Device) -> None:
  
    # Test setup
    encoder_device.step_time(100)

    # When Rotating anti-clockwise Pin B changes before Pin A
    result = encoder_device.set_gpio("A1", 0)
    encoder_device.step_time(100)
    result = encoder_device.set_gpio("A0", 0) 
    encoder_device.step_time(100)

    # Assertions
    cmd = encoder_device.wait_for_cmd_send(
        1, ZCL_CLUSTER_LEVEL_CONTROL, ZCL_CMD_LEVEL_STEP
    )
    assert cmd.data == b'\x01\x0D\x01\x00' # x01 Move Down, StepSize x0D, Transistion Time \x01\x00 (1 x 1/10th Sec)

def test_encoder_cluster_rotating_clockwise_white_pressed_causes_step_up_colour_temp_command(encoder_device: Device) -> None:
  
    # Test setup
    encoder_device.step_time(100)

    # When Rotating clockwise Pin A changes before Pin B
    result = encoder_device.set_gpio("A2", 0) # Press
    encoder_device.step_time(100)
    result = encoder_device.set_gpio("A0", 0)
    encoder_device.step_time(100)
    result = encoder_device.set_gpio("A1", 0) 
    encoder_device.step_time(100)

    # Assertions
    cmd = encoder_device.wait_for_cmd_send(
        1, ZCL_CLUSTER_LIGHTING_COLOR_CONTROL, ZCL_CMD_LIGHTING_COLOR_STEP_TEMP
    )
    assert cmd.data == b'\x01\x0D\x00\x01\x00\x00\x00\xfe\xff' # 01 Move Up, StepSize 0D00, Transistion Time 0100 (1 x 1/10th Sec), 0000 min, feff max

def test_encoder_cluster_rotating_anticlockwise_white_pressed_causes_step_down_colour_temp_command(encoder_device: Device) -> None:
  
    # Test setup
    encoder_device.step_time(100)

    result = encoder_device.set_gpio("A2", 0) # Press
    encoder_device.step_time(100)
    result = encoder_device.set_gpio("A1", 0)
    encoder_device.step_time(100)
    result = encoder_device.set_gpio("A0", 0) 
    encoder_device.step_time(100)

    # Assertions
    cmd = encoder_device.wait_for_cmd_send(
        1, ZCL_CLUSTER_LIGHTING_COLOR_CONTROL, ZCL_CMD_LIGHTING_COLOR_STEP_TEMP
    )
    assert cmd.data == b'\x03\x0D\x00\x01\x00\x00\x00\xfe\xff' # 03 Move Down, StepSize 0D00, Transistion Time 0100 (1 x 1/10th Sec), 0000 min, feff max