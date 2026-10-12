"""BSEED PM metering, protection, converter, and OTA packaging regressions."""

import importlib.util
import math
import re
import subprocess
import sys
from pathlib import Path

import yaml

from client import StubProc
from conftest import Device


BSEED_CONFIG = "b28wrpvx;TS011F-BS-PM;LC3;SB5u;RD2;IB4;M;"
ELECTRICAL = 0x0B04
METERING = 0x0702


def _derive(device: Device, voltage_cv: int, current_ma: int, power_w: int) -> dict:
    result = device.p.exec(
        f"elec_meas_derive {voltage_cv} {current_ma} {power_w}"
    )
    assert result.ok, result.payload
    return {key: int(value) for key, value in result.payload.items()}


def _overload(device: Device, *args) -> dict:
    result = device.p.exec("overload_sim " + " ".join(map(str, args)))
    assert result.ok, result.payload
    return {
        key: int(value)
        for key, value in result.payload.items()
        if value.lstrip("-").isdigit()
    }


def test_bseed_legacy_config_enables_meter_without_mutation():
    db = yaml.safe_load(Path("device_db.yaml").read_text())
    board = db["OUTLET_BSEED_PM_TS011F"]
    assert board["config_str"] == BSEED_CONFIG
    assert (
        board["hlw8012_voltage_multiplier"],
        board["hlw8012_current_multiplier"],
        board["hlw8012_power_multiplier"],
    ) == (161460, 144679, 16989)

    with StubProc(device_config=BSEED_CONFIG) as proc:
        device = Device(proc)
        assert device.read_zigbee_attr(1, ELECTRICAL, 0x0505) == "0"
        assert device.read_zigbee_attr(1, METERING, 0x0300) == "0"


def test_meter_clusters_do_not_overwrite_later_switch_endpoints():
    config = "Stub;Stub;SB5u;SB4u;RD2;EPA1C2B1;M;"
    with StubProc(device_config=config) as proc:
        device = Device(proc)
        assert device.read_zigbee_attr(1, ELECTRICAL, 0x0505) == "0"
        assert device.read_zigbee_attr(2, 0x0012, 0x0055) == "0"


def test_derived_power_quantities():
    with StubProc(device_config=BSEED_CONFIG) as proc:
        result = _derive(Device(proc), 23000, 10000, 2000)
    assert result["apparent_va"] == 2300
    assert result["power_factor"] == 86
    assert abs(result["reactive_var"] - math.isqrt(2300**2 - 2000**2)) <= 1


def test_per_device_calibration_values_persist_in_nvm():
    with StubProc(device_config=BSEED_CONFIG) as proc:
        device = Device(proc)
        device.write_zigbee_attr(1, ELECTRICAL, 0xFF20, "V160000A140000W17000")
        assert (
            device.read_zigbee_attr(1, ELECTRICAL, 0xFF20)
            == "V160000A140000W17000"
        )
    with StubProc(device_config=BSEED_CONFIG) as proc:
        assert (
            Device(proc).read_zigbee_attr(1, ELECTRICAL, 0xFF20)
            == "V160000A140000W17000"
        )


def test_derived_power_clamps_noise_and_handles_no_load():
    with StubProc(device_config=BSEED_CONFIG) as proc:
        device = Device(proc)
        assert _derive(device, 23000, 0, 0) == {
            "apparent_va": 0,
            "reactive_var": 0,
            "power_factor": 0,
        }
        assert _derive(device, 23000, 10000, 2400) == {
            "apparent_va": 2300,
            "reactive_var": 0,
            "power_factor": 100,
        }


def test_overload_peak_and_delayed_trip():
    with StubProc(device_config=BSEED_CONFIG) as proc:
        device = Device(proc)
        _overload(device, "reset")
        peak = _overload(device, 1000, 23000, 5000, 3700, 1, 1)
        assert peak["action"] == 1
        assert peak["alarm"] == 3

        _overload(device, "reset")
        assert _overload(device, 1000, 23000, 100, 3000, 1, 0)["action"] == 0
        assert _overload(device, 30000, 23000, 100, 3000, 1, 0)["action"] == 0
        delayed = _overload(device, 31000, 23000, 100, 3000, 1, 0)
        assert delayed["action"] == 1
        assert delayed["alarm"] == 1


def test_converter_uses_canonical_units_and_readable_measurements():
    output = subprocess.run(
        [sys.executable, "helper_scripts/make_z2m_custom_converters.py", "device_db.yaml"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    start = output.index('"TS011F-BS-PM"')
    end = output.index("\n    {", start)
    bseed = output[start:end]
    for needle in (
        'name: "voltage"', 'unit: "V"', 'divisor: 100',
        'name: "current"', 'unit: "A"', 'divisor: 1000',
        'name: "power"', 'unit: "W"', 'access: "STATE_GET"',
        'name: "energy"', 'unit: "kWh"',
        'name: "calibrate_voltage"', 'romasku.overloadAlarm("overload_alarm"',
    ):
        assert needle in bseed
    assert "response[name] = response[name] / scale" in output
    assert "response[name] = raw === 0 ? 0 : raw / multiplier" in output
    for path in (
        Path("zigbee2mqtt/converters/switch_custom.js"),
        Path("zigbee2mqtt/converters_v1/switch_custom.js"),
    ):
        checked_in = path.read_text()
        checked_start = checked_in.index('"TS011F-BS-PM"')
        checked_end = checked_in.index("\n    {", checked_start)
        checked_bseed = checked_in[checked_start:checked_end]
        assert 'name: "voltage"' in checked_bseed
        assert 'name: "current"' in checked_bseed
        assert 'name: "power"' in checked_bseed
        assert 'access: "STATE_GET"' in checked_bseed


def test_ota_wrappers_keep_identical_payload():
    module_path = Path("src/telink/make_ota.py")
    spec = importlib.util.spec_from_file_location("make_ota", module_path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader
    spec.loader.exec_module(module)

    firmware = bytes(range(64))
    wrappers = [
        module.make_ota_image(firmware, 0x1141, image_type, version, "BSEED")
        for image_type, version in ((43556, 1), (43556, 0xFFFFFFFF), (54179, 0xFFFFFFFF))
    ]
    payload_offset = module.OTA_HDR_STRUCT.size + module.OTA_SUB_ELEMENT_HDR_STRUCT.size
    payloads = [wrapper[payload_offset:] for wrapper in wrappers]
    assert payloads[0] == payloads[1] == payloads[2]


def test_no_load_suppression_contract_is_preserved():
    header = Path("src/base_components/energy_measurement/hlw8012.h").read_text()
    source = Path("src/base_components/energy_measurement/hlw8012.c").read_text()
    assert re.search(r"HLW8012_NO_LOAD_POWER_W\s+2", header)
    assert re.search(r"HLW8012_NO_LOAD_CURRENT_MA\s+50", header)
    assert re.search(r"HLW8012_NO_LOAD_CONFIRM_SAMPLES\s+3", header)
    assert re.search(r"dev->data\.current\s*=\s*0;", source)
    assert re.search(r"dev->data\.power\s*=\s*0;", source)
    assert "if (!dev->data.no_load_suppressed)" in source
