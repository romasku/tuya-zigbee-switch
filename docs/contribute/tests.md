# Testing System

The project uses a comprehensive testing system based on **pytest** and a **stub device implementation** that simulates Zigbee firmware behavior without requiring physical hardware.

## Test Architecture

### Stub Device Implementation

The core of the testing system is the **stub device** (`src/stub/`), which is a host-native implementation of the firmware that:

- Implements the same Hardware Abstraction Layer (HAL) as the real firmware
- Simulates Zigbee network behavior
- Provides a REPL (Read-Eval-Print Loop) interface for interactive control
- Maintains persistent state using NVM simulation in `stub_nvm_data/`
- Supports time control (freezing/stepping time) for deterministic testing

### Test Client Infrastructure

The Python test infrastructure (`tests/client.py`) provides:

- **`StubProc`** class - Manages the stub device subprocess lifecycle
- **Command execution** - Send commands to stub device and receive responses
- **Event handling** - Listen for asynchronous events from the device
- **Process management** - Automatic startup/shutdown of stub processes

### Test Fixtures and Utilities

The test configuration (`tests/conftest.py`) provides reusable fixtures:

- **Device configuration parsing** - Extract pins and endpoints from config strings
- **Hardware abstraction** - Button/relay/LED pin management
- **Zigbee cluster testing** - ZCL command and attribute verification
- **Time control** - Deterministic timing for testing sequences
- **GPIO simulation** - Virtual pin state management

## Test Categories

### 1. Basic Cluster Tests (`test_basic_cluster.py`)

Tests fundamental Zigbee Basic cluster attributes:

```python
def test_manufacturer_name_attr(device: Device) -> None:
    val = device.read_attr(ZCL_CLUSTER_BASIC, ZCL_ATTR_BASIC_MFR_NAME)
    assert val == "StubManufacturer"
```

- Manufacturer information (name, model ID, hardware version)
- Software version and build information
- Date codes and device configuration
- Status LED state management

### 2. Switch Cluster Tests (`test_switch_cluster*.py`)

Tests switch-specific Zigbee cluster behavior:

- **Configuration attributes** - Switch type, actions, modes
- **Multistate input** - Switch state reporting
- **Binding modes** - How switches interact with relays
- **Switch modes** - Toggle vs momentary operation

### 3. Relay Cluster Tests (`test_relay_cluster.py`)

Tests relay control through On/Off cluster:

- On/Off commands and state reporting
- Relay endpoint configuration
- GPIO control for relay outputs

### 4. Device Configuration Tests (`test_device_config_parser.py`)

Tests the configuration string parsing system:

- Pin assignment parsing from `device_db.yaml` entries
- Endpoint generation based on device capabilities
- Configuration validation and error handling

### 5. Network Join Tests (`test_network_join.py`)

Tests Zigbee network behavior:

- Initial join attempts on startup
- LED blinking during join process
- Rejoining after network disconnection
- Status reporting for network state

### 6. Base Components Tests (`test_base_components.py`)

Tests shared utility components used across the firmware.

## Running Tests

### Prerequisites

Install Python dependencies:

```bash
pip install -r requirements.txt
```

Build the stub device:

```bash
make stub/build
```

### Basic Test Execution

Run all tests:

```bash
make tests
# Or directly:
pytest tests/
```

Run specific test file:

```bash
pytest tests/test_switch_cluster.py
```

Run with verbose output:

```bash
pytest -v tests/
```

### Interactive Testing

You can also run the stub device interactively for manual testing:

```bash
make stub/run
# Or with specific device config:
./build/stub/stub_device --device-config "StubMfr;StubDev;LC0;SA0u;RB0;"
```

This provides a REPL interface where you can:

- Check device status: `status`
- Control GPIO pins: `write_pin 12 1`, `read_pin 12`
- Step time forward: `time_step 1000`
- Send ZCL commands: `zcl_attr_read 1 0x0006 0x0000`

## Writing New Tests

### Test Structure Pattern

Follow this pattern for new test files:

```python
import pytest
from tests.conftest import Device, StubProc
from tests.zcl_consts import ZCL_CLUSTER_*, ZCL_ATTR_*

def test_feature_name(stub_proc: StubProc) -> None:
    device = Device(stub_proc)

    # Test setup
    result = device.some_action()

    # Assertions
    assert result == expected_value
```

### Common Test Patterns

**Attribute reading:**

```python
val = device.read_attr(cluster_id, attr_id)
assert val == "expected_value"
```

**GPIO control:**

```python
device.set_gpio("A0", True)
assert device.get_gpio("A0", refresh=True) == True
```

**ZCL command sending:**

```python
device.send_zcl_cmd(endpoint, cluster_id, cmd_id, data_bytes)
```

**Time-based testing:**

```python
device.step_time(500)  # Advance 500ms
```

**Event waiting:**

```python
evt = device.wait_for_event("zcl_cmd", timeout=2.0)
assert evt.payload["cmd"] == "0x01"
```

### Device Configuration

Tests use configuration strings that define device hardware:

```python
@pytest.fixture
def device_config() -> str:
    # Format: "Manufacturer;Device;LED_pin;Switch_pins;Relay_pins;"
    return "StubMfr;StubDev;LC0;SA0u;SA1u;RB0;RB1;"
```

## Test Environment

### Automatic Cleanup

Tests automatically clean up:

- NVM data directory (`stub_nvm_data/`) before and after each test
- Stub processes are terminated after test completion
- GPIO states are reset between tests

### Time Control

Tests can control time for deterministic behavior:

- `freeze_time=True` (default) - Time doesn't advance automatically
- `device.step_time(ms)` - Manually advance time by specified milliseconds
- Useful for testing timeouts, delays, and periodic behavior

### Network State Control

Tests can simulate different network states:

- `joined=True` (default) - Device starts joined to network
- `joined=False` - Device starts unjoined, will attempt to join
- `device.set_network(state)` - Change network state during test
