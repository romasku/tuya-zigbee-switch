let tuyaDefinitions = require("zigbee-herdsman-converters/devices/tuya");
let moesDefinitions = require("zigbee-herdsman-converters/devices/moes");
let avattoDefinitions = require("zigbee-herdsman-converters/devices/avatto");
let tuya = require("zigbee-herdsman-converters/lib/tuya");

// Support Z2M 2.1.3-1
tuyaDefinitions = tuyaDefinitions.definitions ?? tuyaDefinitions;
moesDefinitions = moesDefinitions.definitions ?? moesDefinitions;
avattoDefinitions = avattoDefinitions.definitions ?? avattoDefinitions;

const definitions = [];
const multiplePinoutsDescription = "WARNING! There are multiple known pinouts for this device! Before flashing custom firmware, it is recommended you disassemble the device and trace the board pinout. Please check https://github.com/romasku/tuya-zigbee-switch/tree/main/docs/devices/multiple_pinouts.md";

const ota = require("zigbee-herdsman-converters/lib/ota");

const tuyaModels = [
    "FZB-1",
    "TS0001",
    "TS0001_switch_module",
    "TS0002",
    "TS0002_basic",
    "TS0002_limited",
    "TS0003",
    "TS0003_switch_3_gang",
    "TS0003_switch_module_2",
    "TS0004",
    "TS0004_switch_module",
    "TS0004_switch_module_2",
    "TS0011_switch_module",
    "TS0012",
    "TS0012_switch_module",
    "TS0013",
    "TS0013_switch_module",
    "TS0014",
    "TS011F_plug_1",
    "TS011F_plug_2",
    "TS0726_2_gang_scene_switch",
    "WHD02",
    "_TZ3000_pgq7ormg",
];

const tuyaMultiplePinoutsModels = [
    "TS0004_switch_module_2",
];

for (let definition of tuyaDefinitions) {
    if (tuyaModels.includes(definition.model)) {
        if (tuyaMultiplePinoutsModels.includes(definition.model)) {
            definitions.push(
                {
                    ...definition,
                    description: multiplePinoutsDescription,
                    whiteLabel: definition.whiteLabel.map(entry => ({...entry, description: multiplePinoutsDescription,})),
                    ota: ota.zigbeeOTA,
                }
            )
        }
        else {
            definitions.push(
                {
                    ...definition,
                    ota: ota.zigbeeOTA,
                }
            )
        }
    }
}

const moesModels = [
    "ZS-EUB_1gang",
];

const moesMultiplePinoutsModels = [
];

for (let definition of moesDefinitions) {
    if (moesModels.includes(definition.model)) {
        if (moesMultiplePinoutsModels.includes(definition.model)) {
            definitions.push(
                {
                    ...definition,
                    description: multiplePinoutsDescription,
                    whiteLabel: definition.whiteLabel.map(entry => ({...entry, description: multiplePinoutsDescription,})),
                    ota: ota.zigbeeOTA,
                }
            )
        }
        else {
            definitions.push(
                {
                    ...definition,
                    ota: ota.zigbeeOTA,
                }
            )
        }
    }
}

const avattoModels = [
    "LZWSM16-1",
];

const avattoMultiplePinoutsModels = [
];

for (let definition of avattoDefinitions) {
    if (avattoModels.includes(definition.model)) {
        if (avattoMultiplePinoutsModels.includes(definition.model)) {
            definitions.push(
                {
                    ...definition,
                    description: multiplePinoutsDescription,
                    whiteLabel: definition.whiteLabel.map(entry => ({...entry, description: multiplePinoutsDescription,})),
                    ota: ota.zigbeeOTA,
                }
            )
        }
        else {
            definitions.push(
                {
                    ...definition,
                    ota: ota.zigbeeOTA,
                }
            )
        }
    }
}

module.exports = definitions;
