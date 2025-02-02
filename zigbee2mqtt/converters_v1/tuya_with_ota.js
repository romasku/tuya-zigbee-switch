const tuyaDefinitions = require("zigbee-herdsman-converters/devices/tuya");
const ota = require("zigbee-herdsman-converters/lib/ota");

const tuyaModels = [
    "WHD02",
    "TS0011_switch_module",
    "TS0012_switch_module",
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
