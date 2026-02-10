let tuyaDefinitions = require("zigbee-herdsman-converters/devices/tuya");
let moesDefinitions = require("zigbee-herdsman-converters/devices/moes");
let avattoDefinitions = require("zigbee-herdsman-converters/devices/avatto");
let girierDefinitions = require("zigbee-herdsman-converters/devices/girier");
let lonsonhoDefinitions = require("zigbee-herdsman-converters/devices/lonsonho");
let tuya = require("zigbee-herdsman-converters/lib/tuya");

// Support Z2M 2.1.3-1
tuyaDefinitions = tuyaDefinitions.definitions ?? tuyaDefinitions;
moesDefinitions = moesDefinitions.definitions ?? moesDefinitions;
avattoDefinitions = avattoDefinitions.definitions ?? avattoDefinitions;
girierDefinitions = girierDefinitions.definitions ?? girierDefinitions;
lonsonhoDefinitions = lonsonhoDefinitions.definitions ?? lonsonhoDefinitions;

const definitions = [];


/********************************************************************
  This file (`tuya_with_ota.js`) is generated. 
  
  You can edit it for testing, but for PRs please use:
  - `device_db.yaml`                - add or edit devices
  - `tuya_with_ota.js.jinja`        - update the template
  - `make_z2m_tuya_converters.py`   - update generation script

  Generate with: `make tools/update_converters`
********************************************************************/

const tuyaModels = [
    "FZB-1",
    "QS-Zigbee-SEC01-U",
    "QS-Zigbee-SEC02-U",
    "TS0001",
    "TS0001_switch_1_gang",
    "TS0001_switch_module",
    "TS0001_switch_module_1",
    "TS0002",
    "TS0002_basic",
    "TS0002_limited",
    "TS0003",
    "TS0003_switch_3_gang",
    "TS0003_switch_3_gang_with_backlight",
    "TS0003_switch_module_2",
    "TS0004",
    "TS0004_switch_module",
    "TS0004_switch_module_2",
    "TS0011",
    "TS0011_switch_module",
    "TS0012",
    "TS0012_switch_module",
    "TS0013",
    "TS0013_switch_module",
    "TS0014",
    "TS0044",
    "TS011F_plug_1",
    "TS011F_plug_2",
    "TS0601_switch_1_gang",
    "TS0726_1_gang_scene_switch",
    "TS0726_2_gang_scene_switch",
    "TS0726_3_gang",
    "TS0726_3_gang_scene_switch",
    "TS130F",
    "TW-03",
    "WHD02",
    "_TZ3000_pgq7ormg",
];

for (let definition of tuyaDefinitions) {
    if (tuyaModels.includes(definition.model)) {
        definitions.push(
            {
                ...definition,
                ota: true,
            }
        )
    }
}

const moesModels = [
    "SR-ZS",
    "ZS-EUB_1gang",
];

for (let definition of moesDefinitions) {
    if (moesModels.includes(definition.model)) {
        definitions.push(
            {
                ...definition,
                ota: true,
            }
        )
    }
}

const avattoModels = [
    "LZWSM16-1",
];

for (let definition of avattoDefinitions) {
    if (avattoModels.includes(definition.model)) {
        definitions.push(
            {
                ...definition,
                ota: true,
            }
        )
    }
}

const girierModels = [
    "JR-ZDS01",
];

for (let definition of girierDefinitions) {
    if (girierModels.includes(definition.model)) {
        definitions.push(
            {
                ...definition,
                ota: true,
            }
        )
    }
}

const lonsonhoModels = [
    "TS130F_dual",
];

for (let definition of lonsonhoDefinitions) {
    if (lonsonhoModels.includes(definition.model)) {
        definitions.push(
            {
                ...definition,
                ota: true,
            }
        )
    }
}

module.exports = definitions;
