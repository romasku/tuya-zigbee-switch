const tuyaDefinitions = require("zigbee-herdsman-converters/devices/tuya");

const tuyaModels = [
    "TS0004_switch_module_2",
    "WHD02",
    "TS0001_switch_module",
    "TS0002_basic",
    "TS0002_limited",
    "TS0012_switch_module",
    "TS0011_switch_module",
];

const definitions = [];


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

module.exports = definitions;
