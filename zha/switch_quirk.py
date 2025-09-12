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
    ShortPress = 0x03


class BindedMode(t.enum8):
    Raise = 0x01
    LongPress = 0x02
    ShortPress = 0x03


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

        level_move_rate = ZCLAttributeDef(
            id=0xff04,
            type=t.uint8_t,
            access="rw",
            is_manufacturer_specific=True,
        )

        binded_mode = ZCLAttributeDef(
            id=0xff05,
            type=BindedMode,
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
        pwm_mode: Final = ZCLAttributeDef(
            id=0xff03,
            type=t.Bool,
            access="rw",
            is_manufacturer_specific=True,
        )
        pwm_brightness: Final = ZCLAttributeDef(
            id=0xff04,
            type=t.uint8_t,
            access="rw",
            is_manufacturer_specific=True,
        )

CONFIGS = [
    "jl7qyupf;TS0012-custom;BA0f;LD7;SC2f;SC3f;RC0;RB4;",
    "skueekg3;WHD02-custom;BB4u;LD3;SB5u;RB1;",
    "01gpyda5;TS0002-custom;BD2u;LC2;SB5u;SB4u;RC4;RC3;",
    "bvrlqyj7;TS0002-OXT-CUS;BD2u;LC0;SB4u;SB5u;RC2;RC3;",
    "ji4araar;TS0011-custom;BA0f;LD7;SC2f;RC0;",
    "tqlv4ug4;TS0001-custom;BD2u;LC0;SB4u;RC2;",
    "zmy4lslw;TS0002-GIR;BD2u;LC2;SB5u;RC4;SB4u;RC3;",
    "4rbqgcuv;TS0001-AVB;BC2u;LD2i;SD3u;RC0;",
    "mtnpt6ws;TS0002-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "hbic3ka3;TS0003-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "5ajpkyq6;TS0004-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "18ejxno0;Moes-2-gang;SB6u;SC4u;RB5;RB4;ID3;IC0;M;",
    "18ejxno0;Moes-2-gang-ED;SB6u;SC4u;RB5;RB4;ID3;IC0;M;",
    "f2slq5pj;Bseed-2-gang;SB6u;SA1u;RD3;RC0;IC2;IB4;M;",
    "xk5udnd6;Bseed-2-gang-2;SB5u;SD4u;RC0B6;RA1D7;ID2;ID3;LC3;M;",
    "ljhbw1c9;TS0012-avatto;BB4u;LB5;SC0u;SC3u;RC2;RC4;",
    "avotanj3;TS0013-AVB;BB4u;LB5;SA1u;RC2;SC3u;RC4;SC0u;RD2;",
    "46t1rvdu;WHD02-Aubess;BC4u;LD2;SB4u;RB5;",
    "hhiodade;Moes-1-gang;SC1u;RB5;ID7;M;",
    "qewo8dlz;Moes-3-gang;SB6u;RB5;ID3;SC1u;RB4;ID7;SC4u;RD2;IC0;M;",
    "skueekg3;WHD02-custom;BB1u;LB4;SD2u;RD3;",
    "skueekg3;WHD02-custom;BB1u;LB4;SD2u;RD3;",
    "zmlunnhy;Zemi-2-gang;SC3U;SD2U;IB7;ID7;RB5C4;RC2D4;",
    "hbxsdd6k;TS0011-avatto;BB4u;LB5;SC0u;RC2;",
    "pfc7i3kt;TS0003-custom;BD3u;SC1u;SD7u;SC3u;RB5;RD4;RB4;",
    "7aqaupa9;TS0003-BS;SC3u;SB7u;SB5u;RC2;RB4;RC0;LA0i;M;",
    "pgq7ormg;TS0001-IHS;BC3u;LC2i;SB5u;RD2;",
    "mhhxxjrs;TS0003-IHS;BC3u;LC2i;SD7u;RD2;SB4u;RD3;SB5u;RC0;",
    "knoj8lpk;TS0004-IHS;BC3u;LC2i;SB5u;RD2;SB4u;RD3;SD7u;RC0;SD4u;RC1;",
    "ypgri8yz;ZB08-custom;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "ltt60asa;TS0004-Avv;BB5u;LC1;SB4u;RC0;SD2u;RC4;SC3u;RD4;SC2u;RD7;",
    "mmkbptmx;TS0004-custom;BB6u;LB1;SC1u;RB7;SC2u;RB5;SC3u;RB4;SD2u;RC4;",
    "avky2mvc;Avatto-3-touch;LB5;SD3u;RC2;SD7u;RC3;SD4u;RD2;M;",
]

DEVICE_TYPES = {
    "TS0012-custom": "end_device",
    "WHD02-custom": "router",
    "TS0002-custom": "router",
    "TS0002-OXT-CUS": "router",
    "TS0011-custom": "end_device",
    "TS0001-custom": "router",
    "TS0002-GIR": "router",
    "TS0001-AVB": "router",
    "TS0002-AVB": "router",
    "TS0003-AVB": "router",
    "TS0004-AVB": "router",
    "Moes-2-gang": "router",
    "Moes-2-gang-ED": "end_device",
    "Bseed-2-gang": "end_device",
    "Bseed-2-gang-2": "end_device",
    "TS0012-avatto": "end_device",
    "TS0013-AVB": "end_device",
    "WHD02-Aubess": "end_device",
    "Moes-1-gang": "end_device",
    "Moes-3-gang": "end_device",
    "Zemi-2-gang": "end_device",
    "TS0011-avatto": "end_device",
    "TS0003-custom": "router",
    "TS0003-BS": "router",
    "TS0001-IHS": "router",
    "TS0003-IHS": "router",
    "TS0004-IHS": "router",
    "ZB08-custom": "end_device",
    "TS0004-Avv": "router",
    "TS0004-custom": "router",
    "Avatto-3-touch": "router",
}

for config in CONFIGS:
    zb_manufacturer, zb_model, *peripherals = config.rstrip(";").split(";")
    device_type = DEVICE_TYPES.get(zb_model, "end_device")

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
            .enum(
                CustomOnOffConfigurationCluster.AttributeDefs.binded_mode.name,
                BindedMode,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="binded_mode",
                fallback_name="Binded mode",
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
            .number(
                CustomOnOffConfigurationCluster.AttributeDefs.level_move_rate.name,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="level_move_rate",
                fallback_name="Level move rate",
                min_value=1,
                max_value=255,
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
        
        if device_type == "router":
            builder = (
                builder
                .switch(
                    OnOffWithIndicatorCluster.AttributeDefs.pwm_mode.name,
                    OnOffWithIndicatorCluster.cluster_id,
                    translation_key="relay_led_pwm_mode",
                    fallback_name="Relay LED PWM mode",
                    endpoint_id=endpoint_id,
                )
                .number(
                    OnOffWithIndicatorCluster.AttributeDefs.pwm_brightness.name,
                    OnOffWithIndicatorCluster.cluster_id,
                    translation_key="relay_led_pwm_brightness",
                    fallback_name="Relay LED PWM brightness",
                    min_value=0,
                    max_value=15,
                    step=1,
                    endpoint_id=endpoint_id,
                )
            )



    builder.add_to_registry()
