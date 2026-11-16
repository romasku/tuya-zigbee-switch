// Example Zigbee2MQTT external converter for the countdown timer.
//
// The firmware in this PR implements the ZCL On With Timed Off command
// (0x42) on relay endpoints. Stock zigbee-herdsman-converters turns an
// "on_time" payload into that command, but the generated converters do
// not expose the field, so it cannot be set from the UI.
//
// This file is an example for the four models the implementation covers:
//
//   _TZ3000_ljhbw1c9  LZWSM16-2  TS0012-avatto
//   _TZ3000_kpatq5pq  LZWSM16-2  TS0012-AVB1
//   _TZ3000_mtnpt6ws  ZWSM16-2   TS0002-AVB
//   _TZ3000_4rbqgcuv  ZWSM16-1   TS0001-AVB
//
// The timer was tested on the first two models; the other two build and
// are covered by the same code, but were not verified on hardware.
//
// Install it as an external converter and drop the built-in entry for
// the same model, so the two definitions do not overlap.
//
// Endpoint layout comes from the config_str in device_db.yaml: the switch
// clusters come first, then the relay clusters.
//
//   TS0012-avatto  ljhbw1c9;TS0012-avatto;BB4u;LB5;SC0u;RC2;SC3u;RC4;
//   TS0012-AVB1    kpatq5pq;TS0012-AVB1;BB4u;LB5;SC0u;RC2;SC3u;RC4;
//     switch 1,2   relay 3,4
//   TS0002-AVB     mtnpt6ws;TS0002-AVB;BC2u;LD2i;SD3u;RC0;SD7u;RD4;
//     switch 1,2   relay 3,4
//   TS0001-AVB     4rbqgcuv;TS0001-AVB;BC2u;LD2i;SD3u;RC0;
//     switch 1     relay 2

const {
    deviceEndpoints,
    enumLookup,
    text,
    binary,
    numeric,
    onOff,
} = require("zigbee-herdsman-converters/lib/modernExtend");
const exposes = require("zigbee-herdsman-converters/lib/exposes");
const tz = require("zigbee-herdsman-converters/converters/toZigbee");
const reporting = require("zigbee-herdsman-converters/lib/reporting");
const constants = require("zigbee-herdsman-converters/lib/constants");
const {assertString} = require("zigbee-herdsman-converters/lib/utils");

const {Numeric, access: ea} = exposes;

// ====== romasku attributes (subset used by these models) ======

const romasku = {
    pressAction: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            access: "STATE_GET",
            lookup: {released: 0, press: 1, long_press: 2},
            cluster: "genMultistateInput",
            attribute: "presentValue",
            description: "Action of the switch: 'released' / 'press' / 'long_press'",
            entityCategory: "diagnostic",
        }),
    switchMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: {toggle: 0, momentary: 1, momentary_nc: 2},
            cluster: "genOnOffSwitchCfg",
            attribute: {ID: 0xff00, type: 0x30},
            description: "Select the type of switch connected to the device",
            entityCategory: "config",
        }),
    switchAction: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: {
                on_off: 0,
                off_on: 1,
                toggle_simple: 2,
                toggle_smart_sync: 3,
                toggle_smart_opposite: 4,
            },
            cluster: "genOnOffSwitchCfg",
            attribute: {ID: 0x0010, type: 0x30},
            description: `Select how switch should work:\n|- on_off: physical position 1 = ON, position 2 = OFF\n|- off_on: positions swapped\n|- toggle_simple: any press TOGGLE the relay\n|- toggle_smart_sync: TOGGLE + send ON/OFF to keep binds in sync\n|- toggle_smart_opposite: TOGGLE + send OPPOSITE ON/OFF to binds`,
            entityCategory: "config",
        }),
    relayMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: {detached: 0, press_start: 1, short_press: 3, long_press: 2},
            cluster: "genOnOffSwitchCfg",
            attribute: {ID: 0xff01, type: 0x30},
            description: "When to turn on/off internal relay",
            entityCategory: "config",
        }),
    relayIndex: (name, endpointName, relayCnt) =>
        enumLookup({
            name,
            endpointName,
            lookup: Object.fromEntries(
                Array.from({length: relayCnt || 2}, (_, i) => [`relay_${i + 1}`, i + 1])
            ),
            cluster: "genOnOffSwitchCfg",
            attribute: {ID: 0xff02, type: 0x20},
            description: "Which internal relay this switch triggers",
            entityCategory: "config",
        }),
    bindedMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: {press_start: 1, short_press: 3, long_press: 2},
            cluster: "genOnOffSwitchCfg",
            attribute: {ID: 0xff05, type: 0x30},
            description: "When to send commands to bound devices",
            entityCategory: "config",
        }),
    longPressDuration: (name, endpointName) =>
        numeric({
            name,
            endpointNames: [endpointName],
            cluster: "genOnOffSwitchCfg",
            attribute: {ID: 0xff03, type: 0x21},
            description: "Threshold for long press (ms)",
            valueMin: 0,
            valueMax: 5000,
            entityCategory: "config",
        }),
    levelMoveRate: (name, endpointName) =>
        numeric({
            name,
            endpointNames: [endpointName],
            cluster: "genOnOffSwitchCfg",
            attribute: {ID: 0xff04, type: 0x20},
            description: "Level (dim) move rate in steps/ms",
            valueMin: 1,
            valueMax: 255,
            entityCategory: "config",
        }),
    networkIndicator: (name, endpointName) =>
        binary({
            name,
            endpointName,
            valueOn: ["ON", 1],
            valueOff: ["OFF", 0],
            cluster: "genBasic",
            attribute: {ID: 0xff01, type: 0x10},
            description: "State of the network indicator LED",
            access: "ALL",
            entityCategory: "config",
        }),
    multiPressResetCount: (name, endpointName) =>
        numeric({
            name,
            endpointNames: [endpointName],
            cluster: "genBasic",
            attribute: {ID: 0xff02, type: 0x20},
            description: "Consecutive presses to trigger factory reset (0 = disabled)",
            valueMin: 0,
            valueMax: 255,
            entityCategory: "config",
        }),
    deviceConfig: (name, endpointName) =>
        text({
            name,
            endpointName,
            access: "ALL",
            cluster: "genBasic",
            attribute: {ID: 0xff00, type: 0x44},
            description: "Current configuration of the device",
            zigbeeCommandOptions: {timeout: 30_000},
            validate: assertString,
            entityCategory: "config",
        }),
};

// ====== on_time / off_wait_time in the UI ======
//
// Stock tz.on_off already puts the right bytes on the wire. Only a
// write-only Numeric is needed here, so the field shows up in the UI.

const countdownExposes = (endpointNames) =>
    endpointNames.map((endpointName) =>
        new Numeric("on_time", ea.SET)
            .withUnit("s")
            .withDescription("Seconds to stay ON before switching OFF. 0 = no timer.")
            .withEndpoint(endpointName)
            .withPreset("on", "on", "Turn ON with a countdown timer")
    );

// ====== build the definition ======

const makeDefinition = ({zigbeeModel, model, description, icon, switchNames, relayNames, endpoints}) => {
    // state and on_time need separate converters: Z2M drops a converter
    // bound to endpoints when the payload key carries no endpoint
    // qualifier, and logs "No converter available for 'on_time'".
    const stateConverter = {
        ...tz.on_off,
        key: ["state"],
        endpoints: relayNames,
    };

    const timerConverter = {
        ...tz.on_off,
        key: ["on_time", "off_wait_time"],
        convertSet: async (entity, key, value, meta) => {
            // A qualified state_<relay> key was already handled by the
            // state converter, which sends the command to that endpoint.
            // Sending it again would re-arm the first relay's timer.
            const qualifiedState = Object.keys(meta.message).find(
                (k) => k.startsWith("state_") && relayNames.includes(k.substring("state_".length))
            );
            if (qualifiedState !== undefined) {
                return;
            }

            // A bare on_time carries no state, and tz.on_off rejects a
            // missing one. Default to "on" so the timer branch is taken.
            if (meta.message.state === undefined) {
                meta.message.state = "on";
            }
            return tz.on_off.convertSet(entity, key, value, meta);
        },
    };

    // modernExtend's onOff() provides the state and power_on_behavior
    // exposes and the genOnOff reporting setup. Only its toZigbee list is
    // replaced, to split state from the timer.
    const onOffExtend = onOff({endpointNames: relayNames});
    onOffExtend.toZigbee = [stateConverter, timerConverter, tz.power_on_behavior];

    return {
        zigbeeModel,
        model,
        vendor: "Tuya-custom",
        description,
        icon,

        extend: [
            deviceEndpoints({endpoints}),

            onOffExtend,

            {
                isModernExtend: true,
                exposes: countdownExposes(relayNames),
            },

            // Configuration attributes, on the first switch endpoint.
            romasku.deviceConfig("device_config", switchNames[0]),
            romasku.multiPressResetCount("multi_press_reset_count", switchNames[0]),
            romasku.networkIndicator("network_led", switchNames[0]),

            // Configuration of each physical switch
            ...switchNames.flatMap((sw) => [
                romasku.pressAction(`${sw}_press_action`, sw),
                romasku.switchMode(`${sw}_mode`, sw),
                romasku.switchAction(`${sw}_action_mode`, sw),
                romasku.relayMode(`${sw}_relay_mode`, sw),
                romasku.relayIndex(`${sw}_relay_index`, sw, relayNames.length),
                romasku.bindedMode(`${sw}_binded_mode`, sw),
                romasku.longPressDuration(`${sw}_long_press_duration`, sw),
                romasku.levelMoveRate(`${sw}_level_move_rate`, sw),
            ]),
        ],

        meta: {multiEndpoint: true},

        configure: async (device, coordinatorEndpoint) => {
            // Button press reports (genMultistateInput on the switch endpoints).
            // onOff() above also does the genOnOff binding and reporting: it looks
            // for endpoints by the genOnOff server cluster, and on the switch
            // endpoints genOnOff is a client (is_server = 0 in
            // src/zigbee/switch_cluster.c), so they are skipped.
            for (const sw of switchNames) {
                const endpoint = device.getEndpoint(endpoints[sw]);
                await reporting.bind(endpoint, coordinatorEndpoint, ["genMultistateInput"]);
                await endpoint.configureReporting("genMultistateInput", [
                    {
                        attribute: {ID: 0x0055 /* presentValue */, type: 0x21},
                        minimumReportInterval: 0,
                        maximumReportInterval: constants.repInterval.MAX,
                        reportableChange: 1,
                    },
                ]);
            }
        },

        ota: true,
    };
};


// ====== definitions ======

const twoRelayEndpoints = {switch_left: 1, switch_right: 2, relay_left: 3, relay_right: 4};
const oneRelayEndpoints = {switch: 1, relay: 2};

module.exports = [
    makeDefinition({
        zigbeeModel: ["TS0012-avatto"],
        model: "LZWSM16-2",
        description:
            "AVATTO LZWSM16-2 / TS0012 2-gang switch module, " +
            "romasku/tuya-zigbee-switch firmware with countdown timer support.",
        icon: "https://www.zigbee2mqtt.io/images/devices/LZWSM16-2.png",
        switchNames: ["switch_left", "switch_right"],
        relayNames: ["relay_left", "relay_right"],
        endpoints: twoRelayEndpoints,
    }),
    makeDefinition({
        zigbeeModel: ["TS0012-AVB1"],
        model: "LZWSM16-2",
        description:
            "AVATTO LZWSM16-2 / TS0012 2-gang switch module, " +
            "romasku/tuya-zigbee-switch firmware with countdown timer support.",
        icon: "https://www.zigbee2mqtt.io/images/devices/LZWSM16-2.png",
        switchNames: ["switch_left", "switch_right"],
        relayNames: ["relay_left", "relay_right"],
        endpoints: twoRelayEndpoints,
    }),
    makeDefinition({
        zigbeeModel: ["TS0002-AVB"],
        model: "ZWSM16-2",
        description:
            "AVATTO ZWSM16-2 / TS0002 2-gang switch module, " +
            "romasku/tuya-zigbee-switch firmware with countdown timer support.",
        icon: "https://www.zigbee2mqtt.io/images/devices/ZWSM16-2.png",
        switchNames: ["switch_left", "switch_right"],
        relayNames: ["relay_left", "relay_right"],
        endpoints: twoRelayEndpoints,
    }),
    makeDefinition({
        zigbeeModel: ["TS0001-AVB"],
        model: "ZWSM16-1",
        description:
            "AVATTO ZWSM16-1 / TS0001 1-gang switch module, " +
            "romasku/tuya-zigbee-switch firmware with countdown timer support.",
        icon: "https://www.zigbee2mqtt.io/images/devices/ZWSM16-1.png",
        switchNames: ["switch"],
        relayNames: ["relay"],
        endpoints: oneRelayEndpoints,
    }),
];
