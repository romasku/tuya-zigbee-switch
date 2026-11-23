// Run with the zigbee-herdsman-converters and zigbee-herdsman versions used by Z2M.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const {prepareDefinition} = require("zigbee-herdsman-converters");
const {Zcl} = require("zigbee-herdsman");
const {attributeKeyValue} = require(path.join(path.dirname(require.resolve("zigbee-herdsman")),
    "controller/helpers/zclFrameConverter.js"));

const converter = {exports: {}};
const source = fs.readFileSync(path.join(__dirname, "../zigbee2mqtt/converters/switch_custom.js"), "utf8");
new Function("require", "module", source)(require, converter);

const definitions = converter.exports.map(prepareDefinition);
const model = definitions.find((d) => d.zigbeeModel?.includes("TS0011-avatto"));
const gesture = model.fromZigbee.find((c) => c.cluster === "genMultistateInput" && c.type.length === 1);
assert.deepEqual(gesture.type, ["attributeReport"]);
const device = {customClusters: {}};
const meta = {device, mapped: model, endpoint_name: "switch"};
const expected = {
    2: "single_hold", 3: "position_on", 4: "position_off",
    5: "single", 6: "single_release", 7: "double", 8: "double_hold", 9: "double_release",
    10: "triple", 11: "triple_hold", 12: "triple_release",
};
const eventData = (value) => {
    // Global Report Attributes, server to client, UINT16. Verify herdsman
    // leaves the unregistered manufacturer-specific attribute as a numeric key.
    const bytes = Buffer.from([0x18, 0x01, 0x0a, 0x00, 0xff, 0x21, value, 0]);
    const frame = Zcl.Frame.fromBuffer(0x0012, Zcl.Header.fromBuffer(bytes), bytes, {});
    return attributeKeyValue(frame, 0x1141, {});
};
const publishes = (endpoint, value) => {
    const msg = {endpoint: {ID: endpoint}, data: value === undefined ? {} : eventData(value), type: "attributeReport"};
    return gesture.convert(model, msg);
};
for (const [value, suffix] of Object.entries(expected)) {
    assert.deepEqual(publishes(1, Number(value)), {action: `switch_${suffix}`});
    assert.equal(publishes(2, Number(value)), undefined); // Relay endpoint.
}
for (const value of [undefined, 0, 1, 99]) assert.equal(publishes(1, value), undefined);
for (const value of [0, 1, 2, 3, 4, 8, 11]) {
    const msg = {endpoint: {ID: 1}, data: {presentValue: value}, type: "attributeReport"};
    assert.equal(gesture.convert(model, msg), undefined);
    assert.equal(gesture.convert(model, msg), undefined); // Periodic repeat.
}
const actionExpose = model.exposes.find((e) => e.property === "action");
assert.deepEqual(new Set(actionExpose.values), new Set(Object.values(expected).map((s) => `switch_${s}`)));
assert(model.exposes.some((e) => e.property === "switch_press_action_switch"));

const multi = definitions.find((d) => d.endpoint?.(device).switch_right === 2);
const multiGesture = multi.fromZigbee.find((c) => c.cluster === "genMultistateInput" && c.type.length === 1);
assert.deepEqual(multiGesture.convert(multi, {endpoint: {ID: 2}, data: eventData(7)}), {action: "switch_right_double"});

async function verifyConfig() {
    for (const [name, id, value, type] of [
        ["max_press_count", 0xff07, 2, 0x20],
        ["confirm_release_ms", 0xff08, 200, 0x21],
    ]) {
        assert(model.exposes.some((e) => e.property === `${name}_switch`));
        const tz = model.toZigbee.find((c) => c.key.includes(name));
        const endpoint = {
            ID: 1,
            write: async (cluster, payload) => assert.deepEqual([cluster, payload], ["genOnOffSwitchCfg", {[id]: {value, type}}]),
            read: async (cluster, attrs) => assert.deepEqual([cluster, attrs], ["genOnOffSwitchCfg", [id]]),
        };
        await tz.convertSet(endpoint, name, value, meta);
        await tz.convertGet(endpoint, name, meta);
        const msg = {endpoint, data: {[id]: value}, type: "readResponse"};
        const outputs = model.fromZigbee.filter((c) => c.cluster === "genOnOffSwitchCfg" && c.type.includes(msg.type))
            .map((c) => c.convert(model, msg, undefined, {}, meta));
        assert.deepEqual(outputs.filter(Boolean), [{[`${name}_switch`]: value}]);
    }
}

verifyConfig().then(() => console.log("Switch gesture converter checks passed"), (error) => {
    console.error(error);
    process.exitCode = 1;
});
