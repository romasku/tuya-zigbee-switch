const tuyaDefinitions = require("zigbee-herdsman-converters/devices/tuya");
const ota = require("zigbee-herdsman-converters/lib/ota");

const tuyaModels = [
    "TS0011_switch_module",
    "WHD02",
    "TS0012_switch_module",
    "TS0004_switch_module_2",
    "TS0002_limited",
    "TS0001_switch_module",
    "TS0002_basic",
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

module.exports = definitions;
