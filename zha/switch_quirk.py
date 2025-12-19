from typing import Final

from zhaquirks import CustomCluster
from zigpy.quirks.v2 import QuirkBuilder, ReportingConfig, SensorDeviceClass, EntityType
from zigpy.zcl import ClusterType, foundation
from zigpy.zcl.clusters.general import OnOffConfiguration, SwitchType, MultistateInput, OnOff, Basic
from zigpy.zcl.foundation import ZCLAttributeDef
import zigpy.types as t

class RelayMode(t.enum8):
    Detached = 0x00
    PressStart = 0x01
    LongPress = 0x02
    ShortPress = 0x03

class BindedMode(t.enum8):
    PressStart = 0x01
    LongPress = 0x02
    ShortPress = 0x03

class SwitchActions(t.enum8):
    OnOff = 0x00
    OffOn = 0x01
    ToggleSimple = 0x02
    ToggleSmartSync = 0x03
    ToggleSmartOpposite = 0x04


class SwitchType(t.enum8):
    Toggle = 0x00
    Momentary = 0x01
    Momentary_NC = 0x02


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


class CustomBasicCluster(CustomCluster, Basic):

    class AttributeDefs(foundation.BaseAttributeDefs):

        networkIndicator = ZCLAttributeDef(
            id=0xff01,
            type=t.Bool,
            access="rw",  
            is_manufacturer_specific=True,
        )


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

'''``````````````````````````````````````````````````````````````````
  This file (`zha_quirk.py`) is generated. 
  
  You can edit it for testing, but for PRs please use:
  - `device_db.yaml`                - add or edit devices
  - `switch_quirk.md.jinja`         - update the template
  - `make_zha_quirk.py`             - update generation script

  Generate with: `make quirks`
``````````````````````````````````````````````````````````````````'''

CONFIGS = [
    "46t1rvdu;WHD02-Aubess;BC4u;LD2;SB4u;RB5;",
    "46t1rvdu;WHD02-Aubess-ED;BC4u;LD2;SB4u;RB5;",
    "WHD02-Aubess;WHD02-Aubess;BC4u;LD2;SB4u;RB5;",
    "WHD02-Aubess;WHD02-Aubess-ED;BC4u;LD2;SB4u;RB5;",
    "lmlsduws;TS0002-AUB;BC4u;LB1;SC2u;RB7;SC3u;RB4;",
    "lvhy15ix;TS0003-AUB;BC4u;LB1;SC2u;RB7;SC3u;RB4;SD2u;RB5;",
    "mmkbptmx;TS0004-custom;BB6u;LB1;SC1u;RB7;SC2u;RB5;SC3u;RB4;SD2u;RC4;",
    "Tuya-TS0004-custom;TS0004-custom;BB6u;LB1;SC1u;RB7;SC2u;RB5;SC3u;RB4;SD2u;RC4;",
    "w1tcofu8;TS0001-AVT;LB5;SD7u;RC3;M;",
    "ogpla3lh;TS0002-AVT;LB5;SD3u;RC2;SD4u;RD2;M;",
    "avky2mvc;TS0003-AVT;LB5;SD3u;RC2;SD7u;RC3;SD4u;RD2;M;",
    "avky2mvc;Avatto-3-touch;LB5;SD3u;RC2;SD7u;RC3;SD4u;RD2;M;",
    "Avatto-3-touch;TS0003-AVT;LB5;SD3u;RC2;SD7u;RC3;SD4u;RD2;M;",
    "Avatto-3-touch;Avatto-3-touch;LB5;SD3u;RC2;SD7u;RC3;SD4u;RD2;M;",
    "gzggw2ia;TS0001-AV-DRY;BC2u;LD2i;SD3u;RC0;",
    "4rbqgcuv;TS0001-AVB;BC2u;LD2i;SD3u;RC0;",
    "4rbqgcuv;TS0001-Avatto-custom;BC2u;LD2i;SD3u;RC0;",
    "4rbqgcuv;TS0001-AV-CUS;BC2u;LD2i;SD3u;RC0;",
    "Tuya-TS0001-Avatto-custom;TS0001-AVB;BC2u;LD2i;SD3u;RC0;",
    "Tuya-TS0001-Avatto-custom;TS0001-Avatto-custom;BC2u;LD2i;SD3u;RC0;",
    "Tuya-TS0001-Avatto-custom;TS0001-AV-CUS;BC2u;LD2i;SD3u;RC0;",
    "TS0001-AV-CUS;TS0001-AVB;BC2u;LD2i;SD3u;RC0;",
    "TS0001-AV-CUS;TS0001-Avatto-custom;BC2u;LD2i;SD3u;RC0;",
    "TS0001-AV-CUS;TS0001-AV-CUS;BC2u;LD2i;SD3u;RC0;",
    "TS0001-AVB;TS0001-AVB;BC2u;LD2i;SD3u;RC0;",
    "TS0001-AVB;TS0001-Avatto-custom;BC2u;LD2i;SD3u;RC0;",
    "TS0001-AVB;TS0001-AV-CUS;BC2u;LD2i;SD3u;RC0;",
    "mtnpt6ws;TS0002-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "mtnpt6ws;TS0002-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "mtnpt6ws;TS0002-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "Tuya-TS0002-Avatto-custom;TS0002-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "Tuya-TS0002-Avatto-custom;TS0002-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "Tuya-TS0002-Avatto-custom;TS0002-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "TS0002-AV-CUS;TS0002-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "TS0002-AV-CUS;TS0002-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "TS0002-AV-CUS;TS0002-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "TS0002-AVB;TS0002-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "TS0002-AVB;TS0002-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "TS0002-AVB;TS0002-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;",
    "hbic3ka3;TS0003-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "hbic3ka3;TS0003-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "hbic3ka3;TS0003-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "Tuya-TS0003-Avatto-custom;TS0003-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "Tuya-TS0003-Avatto-custom;TS0003-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "Tuya-TS0003-Avatto-custom;TS0003-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "TS0003-AV-CUS;TS0003-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "TS0003-AV-CUS;TS0003-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "TS0003-AV-CUS;TS0003-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "TS0003-AVB;TS0003-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "TS0003-AVB;TS0003-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "TS0003-AVB;TS0003-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;",
    "5ajpkyq6;TS0004-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "5ajpkyq6;TS0004-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "5ajpkyq6;TS0004-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "Tuya-TS0004-Avatto-custom;TS0004-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "Tuya-TS0004-Avatto-custom;TS0004-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "Tuya-TS0004-Avatto-custom;TS0004-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "TS0004-AV-CUS;TS0004-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "TS0004-AV-CUS;TS0004-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "TS0004-AV-CUS;TS0004-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "TS0004-AVB;TS0004-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "TS0004-AVB;TS0004-Avatto-custom;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "TS0004-AVB;TS0004-AV-CUS;BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;",
    "hbxsdd6k;TS0011-avatto;BB4u;LB5;SC0u;RC2;",
    "hbxsdd6k;TS0011-avatto-ED;BB4u;LB5;SC0u;RC2;",
    "TS0011-Avatto;TS0011-avatto;BB4u;LB5;SC0u;RC2;",
    "TS0011-Avatto;TS0011-avatto-ED;BB4u;LB5;SC0u;RC2;",
    "ljhbw1c9;TS0012-avatto;BB4u;LB5;SC0u;RC2;SC3u;RC4;",
    "ljhbw1c9;TS0012-avatto-ED;BB4u;LB5;SC0u;RC2;SC3u;RC4;",
    "TS0012-Avatto;TS0012-avatto;BB4u;LB5;SC0u;RC2;SC3u;RC4;",
    "TS0012-Avatto;TS0012-avatto-ED;BB4u;LB5;SC0u;RC2;SC3u;RC4;",
    "avotanj3;TS0013-AVB;BB4u;LB5;SA1u;RC2;SC3u;RC4;SC0u;RD2;",
    "b28wrpvx;TS011F-BS-PM;LC3;SB5u;RD2;IB4;M;",
    "o1jzcxou;TS011F-BS;LC2;SB4u;RC3;ID2;M;",
    "jn2x20tg;TS0726-1-BS;LC4;SB1u;RC2;IC0;M;",
    "zjuvw9zf;TS0726-2-BS;LC4;SB1u;RC2;IC0;SB7u;RC3;ID7;M;",
    "iedhxgyi;TS0726-3-BS;LC4;SB1u;RC2;IC0;SB7u;RC3;ID7;SB4u;RD2;IB5;M;",
    "ysdv91bk;TS0001-BSMN;LA0i;SC3u;RC2;M;",
    "aetquff4;BSLR1;LC3;SB4u;RA1D7;IC2;M;SLP;",
    "hafsqare;TS0011-BS-T;LC2;SA0u;RD3;IB4;M;",
    "f2slq5pj;Bseed-2-gang;LC3;SB6u;RD3;IC2;SA1u;RC0;IB4;M;",
    "f2slq5pj;Bseed-2-gang-ED;LC3;SB6u;RD3;IC2;SA1u;RC0;IB4;M;",
    "Bseed-2-gang;Bseed-2-gang;LC3;SB6u;RD3;IC2;SA1u;RC0;IB4;M;",
    "Bseed-2-gang;Bseed-2-gang-ED;LC3;SB6u;RD3;IC2;SA1u;RC0;IB4;M;",
    "xk5udnd6;Bseed-2-gang-2;LC3;SB5u;RC0B6;ID2;SD4u;RA1D7;ID3;M;SLP;",
    "xk5udnd6;Bseed-2-gang-2-ED;LC3;SB5u;RC0B6;ID2;SD4u;RA1D7;ID3;M;SLP;",
    "Bseed-2-gang-2;Bseed-2-gang-2;LC3;SB5u;RC0B6;ID2;SD4u;RA1D7;ID3;M;SLP;",
    "Bseed-2-gang-2;Bseed-2-gang-2-ED;LC3;SB5u;RC0B6;ID2;SD4u;RA1D7;ID3;M;SLP;",
    "e98krvvk;Bseed-2-gang-3;LC3;SB6u;RD3;IC2;SA1u;RC0;IB4;M;",
    "5e5ptb24;TS0013-BS;LD2;SB6u;RD3;IC3;SA0u;RD7;IC2;SA1u;RC0;IB5;M;",
    "9akmi5ly;TS0001-CUS-T;LB4i;SC2u;RC3;ID2i;M;",
    "blhvsaqf;TS0001-BS-T;LC4;SC1u;RC3;IC2;M;",
    "cauq1okq;TS0002-CUS-T;LC4;SB5u;RC2;IB4i;SD2u;RC3;M;",
    "l9brjwau;TS0002-BS-1;LC4;SB5u;RD3;IC3;SD3u;RC2;ID7;M;",
    "7aqaupa9;TS0003-BS;LA0i;SC3u;RC2;SB7u;RB4;SB5u;RC0;M;",
    "qkixdnon;TS0003-BSEED;LC4i;SB5u;RC3;SC1u;RC2;SD3u;RD7;M;",
    "kfwhmnvc;TS0013-2-BS;LC3;SB5u;RC0B6;ID2;SB4u;RA1D7;IC2;SD4u;RA0C1;ID3;M;SLP;",
    "s6ma1nh4;TS0004-BS;LA0i;SC3u;RC2;SB7u;RB4;SB5u;RC0;SD7u;RD2;M;",
    "num4zd6s;TS0012-EKF;BA0u;LD7;SC2u;RC0;SC3u;RB4;",
    "hyziup76;TS0001-GS;BA0u;LC0;SB4u;RC2;",
    "wxtmgjbd;TS0002-GS;BA0u;LC0;SB4u;RC2;SB5u;RC3;",
    "zw7yf6yk;TS0001-GIR;BB1u;LC3i;SB5u;RD2;",
    "6axxqqi2;TS0001-GIR-1;BC2u;LB5i;SB4u;RD2;",
    "zmy4lslw;TS0002-GIR;BD2u;LC2;SB5u;RC4;SB4u;RC3;",
    "zmy4lslw;TS0002-custom;BD2u;LC2;SB5u;RC4;SB4u;RC3;",
    "Tuya-TS0002-custom;TS0002-GIR;BD2u;LC2;SB5u;RC4;SB4u;RC3;",
    "Tuya-TS0002-custom;TS0002-custom;BD2u;LC2;SB5u;RC4;SB4u;RC3;",
    "odzoiovu;TS0003-GR;BB1u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "ypgri8yz;ZB08-custom;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "ypgri8yz;ZB08-custom-ED;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "Girier-ZB08-custom;ZB08-custom;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "Girier-ZB08-custom;ZB08-custom-ED;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "Girier-ZB08-custom-ED;ZB08-custom;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "Girier-ZB08-custom-ED;ZB08-custom-ED;BA0u;LD7;SC2u;RC0;SC3u;RB4;SD2u;RB5;",
    "tw4ztbp4;TS0011-HOMMYN;BA0u;LD7;SC2u;RB5;",
    "mh9px7cq;TS0044-CUS;LC0;SD2u;RD4;SC2u;RA0;SC3u;RD7;SB4u;RB5;M;",
    "qq9ahj6z;TS0001-IHS-T;LC4i;SB4U;RC3;M;",
    "zxrfobzw;TS0002-IHS-T;LC4i;SC0U;RC2;SB5U;RD2;M;",
    "pgq7ormg;TS0001-IHS;BC3u;LC2i;SB5u;RD2;",
    "mhhxxjrs;TS0003-IHS;BC3u;LC2i;SD7u;RD2;SB4u;RD3;SB5u;RC0;",
    "mhhxxjrs;TS0003-3CH-cus;BC3u;LC2i;SD7u;RD2;SB4u;RD3;SB5u;RC0;",
    "TS0003-3CH-cus;TS0003-IHS;BC3u;LC2i;SD7u;RD2;SB4u;RD3;SB5u;RC0;",
    "TS0003-3CH-cus;TS0003-3CH-cus;BC3u;LC2i;SD7u;RD2;SB4u;RD3;SB5u;RC0;",
    "TS0003-IHS;TS0003-IHS;BC3u;LC2i;SD7u;RD2;SB4u;RD3;SB5u;RC0;",
    "TS0003-IHS;TS0003-3CH-cus;BC3u;LC2i;SD7u;RD2;SB4u;RD3;SB5u;RC0;",
    "knoj8lpk;TS0004-IHS;BC3u;LC2i;SB5u;RD2;SB4u;RD3;SD7u;RC0;SD4u;RC1;",
    "TS0004-IHS;TS0004-IHS;BC3u;LC2i;SB5u;RD2;SB4u;RD3;SD7u;RC0;SD4u;RC1;",
    "tqwydnqn;TS0013-MH;SC4u;RB4A0;ID2;SD7u;RD4B5;IC3;SB7u;RC0C2;IB1;M;",
    "imaccztn;TS0004-MC;LC3i;SD7u;RD4;SC0u;RA0;SB5u;RD2;SB7u;RC2;M;",
    "u3oupgdy;TS0004-MC2;LC3i;SD7u;RD4;SC0u;RA0;SB5u;RD2;SB7u;RC2;M;",
    "hhiodade;Moes-1-gang;SC1u;RB5;ID7;M;",
    "hhiodade;Moes-1-gang-ED;SC1u;RB5;ID7;M;",
    "Moes-1-gang;Moes-1-gang;SC1u;RB5;ID7;M;",
    "Moes-1-gang;Moes-1-gang-ED;SC1u;RB5;ID7;M;",
    "18ejxno0;Moes-2-gang;SB6u;RB5;ID3;SC4u;RB4;IC0;M;",
    "18ejxno0;Moes-2-gang-ED;SB6u;RB5;ID3;SC4u;RB4;IC0;M;",
    "Moes-2-gang;Moes-2-gang;SB6u;RB5;ID3;SC4u;RB4;IC0;M;",
    "Moes-2-gang;Moes-2-gang-ED;SB6u;RB5;ID3;SC4u;RB4;IC0;M;",
    "qewo8dlz;Moes-3-gang;SB6u;RB5;ID3;SC1u;RB4;ID7;SC4u;RD2;IC0;M;",
    "qewo8dlz;Moes-3-gang-ED;SB6u;RB5;ID3;SC1u;RB4;ID7;SC4u;RD2;IC0;M;",
    "Moes-3-gang;Moes-3-gang;SB6u;RB5;ID3;SC1u;RB4;ID7;SC4u;RD2;IC0;M;",
    "Moes-3-gang;Moes-3-gang-ED;SB6u;RB5;ID3;SC1u;RB4;ID7;SC4u;RD2;IC0;M;",
    "mrduubod;MS4;SB1u;RA4;ID0;SB0u;RC0;IC2;SA3u;RC1;IA5;SA0u;RD1;IA6;M;",
    "qaa59zqd;TS0002-MS;BB1u;LC3;SB5u;RD2;SB4u;RC2;",
    "pfc7i3kt;TS0003-custom;BD3u;SC1u;RB5;SD7u;RD4;SC3u;RB4;",
    "Tuya-TS0003-custom;TS0003-custom;BD3u;SC1u;RB5;SD7u;RD4;SC3u;RB4;",
    "ruxexjfz;TS0002-NS;BB5u;LB4;SB7u;RD2;SB1u;RC3;",
    "hdc8bbha;NovatoZRM01;BB4u;LC2;SC4f;RD2;",
    "m8f3z8ju;NovatoZRM02;BC3u;LC4;SC2f;RB5;SB4f;RD2;",
    "30jqysvd;NovatoZNR01;BB7u;LB1;SC2u;RB5;",
    "bvrlqyj7;TS0002-OXT-CUS;BD2u;LC0;SB4u;RC2;SB5u;RC3;",
    "TS0002-OXT-CUS;TS0002-OXT-CUS;BD2u;LC0;SB4u;RC2;SB5u;RC3;",
    "c4muk4ys;TS0012-QS;BB4u;LC2;SD2u;RA0B6;SC3u;RC0D7;SLP;",
    "u6ocpapf;TS0001-CUS;LB1;SC3u;RD2;M;",
    "gbdxbmwz;TS0004-CUS;LB1;SC3u;RD2;SD7u;RB5;SC2u;RB4;SB7u;RC0;M;",
    "tqlv4ug4;TS0001-custom;BD2u;LC0;SB4u;RC2;",
    "Tuya-TS0001-custom;TS0001-custom;BD2u;LC0;SB4u;RC2;",
    "skueekg3;WHD02-custom;BB4u;LD3;SB5u;RB1;",
    "Tuya-WHD02-custom;WHD02-custom;BB4u;LD3;SB5u;RB1;",
    "skueekg3;WHD02-custom;BB1u;LB4i;SD2u;RD3;",
    "Tuya-WHD02-custom;WHD02-custom;BB1u;LB4i;SD2u;RD3;",
    "skueekg3;WHD02-custom;BB1u;LB4i;SD2u;RD3;",
    "Tuya-WHD02-custom;WHD02-custom;BB1u;LB4i;SD2u;RD3;",
    "ZG-301Z;TS0001-HOB;BB1u;LD4i;SB6u;RA1;",
    "npzfdcof;TS0001-TLED;BD2u;LC3i;SB5u;RB4;",
    "01gpyda5;TS0002-custom;BD2u;LC2;SB5u;RC4;SB4u;RC3;",
    "Tuya-TS0002-custom;TS0002-custom;BD2u;LC2;SB5u;RC4;SB4u;RC3;",
    "ltt60asa;TS0004-Avv;BB5u;LC1;SB4u;RC0;SD2u;RC4;SC3u;RD4;SC2u;RD7;",
    "TS0004-Avv;TS0004-Avv;BB5u;LC1;SB4u;RC0;SD2u;RC4;SC3u;RD4;SC2u;RD7;",
    "ji4araar;TS0011-custom;BA0f;LD7;SC2f;RC0;",
    "Tuya-TS0011-custom;TS0011-custom;BA0f;LD7;SC2f;RC0;",
    "ji4araar;TS0011-CUS-2;BA0u;LD7;SC2u;RC0;",
    "jl7qyupf;TS0012-custom;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "jl7qyupf;TS0042-CUSTOM;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "jl7qyupf;TS0012-custom-end-device;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "Tuya-CUSTOM;TS0012-custom;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "Tuya-CUSTOM;TS0042-CUSTOM;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "Tuya-CUSTOM;TS0012-custom-end-device;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "Tuya-TS0012-custom;TS0012-custom;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "Tuya-TS0012-custom;TS0042-CUSTOM;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "Tuya-TS0012-custom;TS0012-custom-end-device;BA0f;LD7;SC2f;RC0;SC3f;RB4;",
    "zmlunnhy;Zemi-2-gang;SC3u;RC2D4;IB7;SD2u;RB5C4;ID7;M;",
    "zmlunnhy;Zemi-2-gang-ED;SC3u;RC2D4;IB7;SD2u;RB5C4;ID7;M;",
    "Zemi-2-gang;Zemi-2-gang;SC3u;RC2D4;IB7;SD2u;RB5C4;ID7;M;",
    "Zemi-2-gang;Zemi-2-gang-ED;SC3u;RC2D4;IB7;SD2u;RB5C4;ID7;M;",
    "TUYA;DEV-ZTU2;LD7;SA0u;RC1;IB6;M;",
    "sonoff;ZBMINIL2-custom;BA0u;LC5i;SA6u;RA5A4;",
]

for config in CONFIGS:
    zb_manufacturer, zb_model, *peripherals = config.rstrip(";").split(";")

    relay_cnt = 0
    switch_cnt = 0
    indicators_cnt = 0
    has_dedicated_net_led = False
    for peripheral in peripherals:
        if peripheral == "SLP":
            continue
        if peripheral[0] == "R":
            relay_cnt += 1
        if peripheral[0] == 'S':
            switch_cnt += 1
        if peripheral[0] == 'I':
            indicators_cnt += 1
        if peripheral[0] == 'L':
            has_dedicated_net_led = True

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
                translation_key="switch_actions_"+str(endpoint_id),
                fallback_name="Switch actions "+str(endpoint_id),
                endpoint_id=endpoint_id,
                # Next is hack to force binding to make all attrs values visible.
                # TODO: find a better approach
                reporting_config=ReportingConfig(min_interval=0, max_interval=300, reportable_change=1),
            )
            .enum(
                CustomOnOffConfigurationCluster.AttributeDefs.switch_mode.name,
                SwitchType,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="switch_mode_"+str(endpoint_id),
                fallback_name="Switch mode "+str(endpoint_id),
                endpoint_id=endpoint_id,
            )
            .enum(
                CustomOnOffConfigurationCluster.AttributeDefs.relay_mode.name,
                RelayMode,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="relay_mode_"+str(endpoint_id),
                fallback_name="Relay mode "+str(endpoint_id),
                endpoint_id=endpoint_id,
            )
            .number(
                CustomOnOffConfigurationCluster.AttributeDefs.relay_index.name,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="relay_index_"+str(endpoint_id),
                fallback_name="Relay index "+str(endpoint_id),
                min_value=1,
                max_value=relay_cnt,
                step=1,
                endpoint_id=endpoint_id,
            )
            .enum(
                CustomOnOffConfigurationCluster.AttributeDefs.binded_mode.name,
                BindedMode,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="binded_mode_"+str(endpoint_id),
                fallback_name="Binded mode "+str(endpoint_id),
                endpoint_id=endpoint_id,
            )
            .number(
                CustomOnOffConfigurationCluster.AttributeDefs.long_press_duration.name,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="long_press_duration_"+str(endpoint_id),
                fallback_name="Long press mode "+str(endpoint_id),
                min_value=0,
                max_value=5000,
                step=1,
                endpoint_id=endpoint_id,
            )
            .number(
                CustomOnOffConfigurationCluster.AttributeDefs.level_move_rate.name,
                CustomOnOffConfigurationCluster.cluster_id,
                translation_key="level_move_rate_"+str(endpoint_id),
                fallback_name="Level move rate "+str(endpoint_id),
                min_value=1,
                max_value=255,
                step=1,
                endpoint_id=endpoint_id,
            )
            .sensor(
                MultistateInput.AttributeDefs.present_value.name,
                MultistateInput.cluster_id,
                translation_key="press_action_"+str(endpoint_id),
                fallback_name="Press action "+str(endpoint_id),
                endpoint_id=endpoint_id,
                reporting_config=ReportingConfig(min_interval=0, max_interval=300, reportable_change=1),
                device_class=SensorDeviceClass.ENUM,
                attribute_converter = lambda x: {0: "released", 1: "press", 2: "long_press", 3: "position_on", 4: "position_off"}[int(x)]
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
                translation_key="relay_led_mode_"+str(endpoint_id),
                fallback_name="Relay Led mode "+str(endpoint_id),
                endpoint_id=endpoint_id
            )
            .switch(
                OnOffWithIndicatorCluster.AttributeDefs.led_state.name,
                OnOffWithIndicatorCluster.cluster_id,
                translation_key="relay_led_state_"+str(endpoint_id),
                fallback_name="Relay led state "+str(endpoint_id),
                endpoint_id=endpoint_id,
                reporting_config=ReportingConfig(
                    min_interval=0, max_interval=300, reportable_change=1
                ),
            )
        )

    if has_dedicated_net_led:
        builder = (
            builder
            .removes(Basic.cluster_id, cluster_type=ClusterType.Client, endpoint_id=1)
            .adds(CustomBasicCluster, endpoint_id=1)
            .switch(
                CustomBasicCluster.AttributeDefs.networkIndicator.name,
                CustomBasicCluster.cluster_id,
                translation_key="network_indicator",
                fallback_name="Network indicator",
                endpoint_id=1,
                entity_type=EntityType.CONFIG,
            )
        )

    builder.add_to_registry()
