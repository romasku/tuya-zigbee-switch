import pytest

from helper_scripts.check_avatto_end_device import make_check_payload


ADDRESS = "0x0011223344556677"
END_DEVICE = {
    "ieee_address": ADDRESS,
    "type": "EndDevice",
    "interview_completed": True,
    "model_id": "TS0011-avatto",
    "manufacturer": "hbxsdd6k",
    "software_build_id": "1.1.3-ec57f81b",
}


def test_generates_only_single_target_check_payload():
    router = {**END_DEVICE, "type": "Router", "ieee_address": "0x0000000000000000"}
    assert make_check_payload([router, END_DEVICE], ADDRESS) == {
        "id": ADDRESS, "url": "avatto-phase1/index.json",
    }


@pytest.mark.parametrize("changes", [
    {"type": "Router"}, {"type": None}, {"interview_completed": False},
    {"model_id": "TS0011"}, {"manufacturer": "other"},
    {"software_build_id": None}, {"ieee_address": "0x0000000000000000"},
])
def test_rejects_wrong_or_unverified_targets(changes):
    with pytest.raises(ValueError):
        make_check_payload([{**END_DEVICE, **changes}], ADDRESS)
