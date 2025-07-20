let tuyaDefinitions = require("zigbee-herdsman-converters/devices/tuya");
let moesDefinitions = require("zigbee-herdsman-converters/devices/moes");
let avattoDefinitions = require("zigbee-herdsman-converters/devices/avatto");

// Support Z2M 2.1.3-1
tuyaDefinitions = tuyaDefinitions.definitions ?? tuyaDefinitions;
moesDefinitions = moesDefinitions.definitions ?? moesDefinitions;
avattoDefinitions = avattoDefinitions.definitions ?? avattoDefinitions;

const ota = require("zigbee-herdsman-converters/lib/ota");

const tuyaModels = [
    "TS0001_switch_module",
    "TS0002_basic",
    "TS0002_limited",
    "TS0003",
    "TS0003_switch_3_gang",
    "TS0003_switch_module_2",
    "TS0004_switch_module",
    "TS0004_switch_module_2",
    "TS0011_switch_module",
    "TS0012",
    "TS0012_switch_module",
    "TS0013",
    "TS0013_switch_module",
    "WHD02",
];

const definitions = [];


for (let definition of tuyaDefinitions) {
    if (tuyaModels.includes(definition.model)) {
        definitions.push(
            {
                ...definition,
                ota: ota.zigbeeOTA,
            }
        )
    }
}

const moesModels = [
    "ZS-EUB_1gang",
];

for (let definition of moesDefinitions) {
    if (moesModels.includes(definition.model)) {
        definitions.push(
            {
                ...definition,
                ota: ota.zigbeeOTA,
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
                ota: ota.zigbeeOTA,
            }
        )
    }
}

module.exports = definitions;
