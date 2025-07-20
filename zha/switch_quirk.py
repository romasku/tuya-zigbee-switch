from typing import Final

from zhaquirks import CustomCluster
from zigpy.quirks.v2 import QuirkBuilder, ReportingConfig, SensorDeviceClass
from zigpy.zcl import ClusterType, foundation
from zigpy.zcl.clusters.general import OnOffConfiguration, SwitchType, SwitchActions, MultistateInput, OnOff
from zigpy.zcl.foundation import ZCLAttributeDef
import zigpy.types as t


class RelayMode(t.enum8):
    Detached = 0x00
    Raise = 0x01
    LongPress = 0x02


class CustomOnOffConfigurationCluster(CustomCluster, OnOffConfiguration):

    class AttributeDefs(OnOffConfiguration.AttributeDefs):
        """Attribute Definitions."""

        switch_mode = ZCLAttributeDef(
            id=0xff00,
            type=SwitchType,
            access="rw",
            is_manufacturer_specific=True,
        )

        relay_mode = ZCLAttributeDef(
            id=0xff01,
            type=RelayMode,
            access="rw",
            is_manufacturer_specific=True,
        )

        relay_index = ZCLAttributeDef(
            id=0xff02,
            type=t.uint8_t,
            access="rw",
            is_manufacturer_specific=True,
        )

        long_press_duration = ZCLAttributeDef(
            id=0xff03,
            type=t.uint16_t,
            access="rw",
            is_manufacturer_specific=True,
        )


class CustomMultistateInputCluster(CustomCluster, MultistateInput):

    class AttributeDefs(foundation.BaseAttributeDefs):
        present_value: Final = ZCLAttributeDef(
            id=0x0055, type=t.uint16_t, access="r*w", mandatory=True
        )
        cluster_revision: Final = foundation.ZCL_CLUSTER_REVISION_ATTR
        reporting_status: Final = foundation.ZCL_REPORTING_STATUS_ATTR


class RelayIndicatorMode(t.enum8):
    Same = 0x00
    Opposite = 0x01
    Manual = 0x02


class OnOffWithIndicatorCluster(CustomCluster, OnOff):

    class AttributeDefs(OnOff.AttributeDefs):
        led_mode: Final = ZCLAttributeDef(
            id=0xff01,
            type=RelayIndicatorMode,
            access="rw",
            is_manufacturer_specific=True,
        )
        led_state: Final = ZCLAttributeDef(
            id=0xff02,
            type=t.Bool,
            access="rw",
            is_manufacturer_specific=True,
        )

CONFIGS = [
    "Tuya-TS0012-custom;TS0012-custom;BA0f;LD7;SC2f;SC3f;RC0;RB4;",
    "Tuya-TS0012-custom;TS0012-custom-end-device;BA0f;LD7;SC2f;SC3f;RC0;RB4;",
    "Tuya-WHD02-custom;WHD02-custom;BB4u;LD3;SB5u;RB1;",
    "Tuya-TS0002-custom;TS0002-custom;BD2u;LC2;SB5u;SB4u;RC4;RC3;",
    "Tuya-TS0011-custom;TS0011-custom;BA0f;LD7;SC2f;RC0;",
    "Tuya-TS0011-custom;TS0011-custom;BA0f;LD7;SC2f;RC0;",
    "Tuya-TS0001-custom;TS0001-custom;BD2u;LC0;SB4u;RC2;",
    "Tuya-TS0002-custom;TS0002-custom;BD2u;LC2;SB5u;RC4;SB4u;RC3;",
    "TS0001-AV-CUS;TS0001-AV-CUS;BC2u;LD2i;SD3u;RC0;",
    "TS0002-AV-CUS;TS0002-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "TS0003-AV-CUS;TS0003-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "TS0004-AV-CUS;TS0004-AV-CUS;BC2u;LD2i;SD7u;RB6;SC0u;RD4;SA0u;RC1;SA1u;RC4;",
    "Moes-2-gang;Moes-2-gang;SB6u;SC4u;RB5;RB4;ID3;IC0;M;",
    "Moes-2-gang;Moes-2-gang-ED;SB6u;SC4u;RB5;RB4;ID3;IC0;M;",
    "Bseed-2-gang;Bseed-2-gang;SB6u;SA1u;RD3;RC0;IC2;IB4;M;",
    "Bseed-2-gang;Bseed-2-gang-ED;SB6u;SA1u;RD3;RC0;IC2;IB4;M;",
    "Bseed-2-gang-2;Bseed-2-gang-2;SB5u;SD4u;RC0B6;RA1D7;ID2;ID3;LC3;M;",
    "Bseed-2-gang-2;Bseed-2-gang-2-ED;SB5u;SD4u;RC0B6;RA1D7;ID2;ID3;LC3;M;",
    "TS0012-Avatto;TS0012-avatto;BB4f;LB5;SC0f;SC3f;RC2;RC4;",
    "TS0012-Avatto;TS0012-avatto-ED;BB4f;LB5;SC0f;SC3f;RC2;RC4;",
    "WHD02-Aubess;WHD02-Aubess;BC4u;LD2;SB4u;RB5;",
    "WHD02-Aubess;WHD02-Aubess-ED;BC4u;LD2;SB4u;RB5;",
    "Moes-1-gang;Moes-1-gang;SC1u;RB5;ID7;M;",
    "Moes-1-gang;Moes-1-gang-ED;SC1u;RB5;ID7;M;",
    "Moes-3-gang;Moes-3-gang;SB6u;RB5;ID3;SC1u;RB4;ID7;SC4u;RD2;IC0;M;",
    "Moes-3-gang;Moes-3-gang-ED;SB6u;RB5;ID3;SC1u;RB4;ID7;SC4u;RD2;IC0;M;",
    "Tuya-WHD02-custom;WHD02-custom;BB1u;LB4;SD2u;RD3;",
    "Tuya-WHD02-custom;WHD02-custom;BB1u;LB4;SD2u;RD3;",
    "Zemi-2-gang;Zemi-2-gang;SC3U;SD2U;IB7;ID7;RB5C4;RC2D4;",
    "Zemi-2-gang;Zemi-2-gang-ED;SC3U;SD2U;IB7;ID7;RB5C4;RC2D4;",
    "TS0011-Avatto;TS0011-avatto;BB4u;LB5;SC0u;RC2;",
    "TS0011-Avatto;TS0011-avatto-ED;BB4u;LB5;SC0u;RC2;",
    "Tuya-TS0003-custom;TS0003-custom;BD3u;SC1u;SD7u;SC3u;RB5;RD4;RB4;",
    "Girier-ZB08-custom;ZB08-custom;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "Girier-ZB08-custom-ED;ZB08-custom-ED;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "TS0004-Avv;TS0004-Avv;BB5u;LC1;SB4u;RC0;SD2u;RC4;SC3u;RD4;SC2u;RD7;",
    "Tuya-TS0004-custom;TS0004-custom;BB6u;LB1;SC1u;RB7;SC2u;RB5;SC3u;RB4;SD2u;RC4;",
    "Avatto-3-touch;Avatto-3-touch;LB5;SD3u;RC2;SD7u;RC3;SD4u;RD2;M;",
]

for config in CONFIGS:
    zb_manufacturer, zb_model, *peripherals = config.rstrip(";").split(";")

    relay_cnt = 0
    switch_cnt = 0
    indicators_cnt = 0
    for peripheral in peripherals:
        if peripheral[0] == "R":
            relay_cnt += 1
        if peripheral[0] == 'S':
            switch_cnt += 1
        if peripheral[0] == 'I':
            indicators_cnt += 1

    builder =  QuirkBuilder(zb_manufacturer, zb_model)

    for endpoint_id in range(1, switch_cnt + 1):
        builder = (
            builder
            .removes(OnOffConfiguration.cluster_id, cluster_type=ClusterType.Client, endpoint_id=endpoint_id)
            .adds(CustomOnOffConfigurationCluster, endpoint_id=endpoint_id)
            .removes(MultistateInput.cluster_id, cluster_type=ClusterType.Client, endpoint_id=endpoint_id)
            .adds(CustomMultistateInputCluster, endpoint_id=endpoint_id)
            .enum(
                CustomOnOffConfigurationCluster.AttributeDefs.switch_actions.name,
                SwitchActions,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="switch_actions",
                fallback_name="Switch actions",
                endpoint_id=endpoint_id,
                # Next is hack to force binding to make all attrs values visible.
                # TODO: find a better approach
                reporting_config=ReportingConfig(min_interval=0, max_interval=300, reportable_change=1),
            )
            .enum(
                CustomOnOffConfigurationCluster.AttributeDefs.switch_mode.name,
                SwitchType,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="switch_mode",
                fallback_name="Switch mode",
                endpoint_id=endpoint_id,
            )
            .enum(
                CustomOnOffConfigurationCluster.AttributeDefs.relay_mode.name,
                RelayMode,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="relay_mode",
                fallback_name="Relay mode",
                endpoint_id=endpoint_id,
            )
            .number(
                CustomOnOffConfigurationCluster.AttributeDefs.relay_index.name,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="relay_index",
                fallback_name="Relay index",
                min_value=1,
                max_value=relay_cnt,
                step=1,
                endpoint_id=endpoint_id,
            )
            .number(
                CustomOnOffConfigurationCluster.AttributeDefs.long_press_duration.name,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="long_press_duration",
                fallback_name="Long press mode",
                min_value=0,
                max_value=5000,
                step=1,
                endpoint_id=endpoint_id,
            )
            .sensor(
                MultistateInput.AttributeDefs.present_value.name,
                MultistateInput.cluster_id,
                translation_key="press_action",
                fallback_name="Press action",
                endpoint_id=endpoint_id,
                reporting_config=ReportingConfig(min_interval=0, max_interval=300, reportable_change=1),
                device_class=SensorDeviceClass.ENUM,
                attribute_converter = lambda x: {0: "released", 1: "press", 2: "long_press"}[int(x)]
            )
        )
    for endpoint_id in range(switch_cnt + 1, switch_cnt + indicators_cnt + 1):
        builder = (
            builder
            .removes(OnOff.cluster_id, cluster_type=ClusterType.Client, endpoint_id=endpoint_id)
            .adds(OnOffWithIndicatorCluster, endpoint_id=endpoint_id)
            .enum(
                OnOffWithIndicatorCluster.AttributeDefs.led_mode.name,
                RelayIndicatorMode,
                OnOffWithIndicatorCluster.cluster_id,
                translation_key="relay_led_mode",
                fallback_name="Relay Led mode",
                endpoint_id=endpoint_id
            )
            .switch(
                OnOffWithIndicatorCluster.AttributeDefs.led_state.name,
                OnOffWithIndicatorCluster.cluster_id,
                translation_key="relay_led_state",
                fallback_name="Relay led state",
                endpoint_id=endpoint_id,
            )
        )



    builder.add_to_registry()
