const {
    numeric,
    enumLookup,
    deviceEndpoints,
    onOff,
    text,
    binary,
} = require("zigbee-herdsman-converters/lib/modernExtend");
const {assertString} = require("zigbee-herdsman-converters/lib/utils");
const reporting = require("zigbee-herdsman-converters/lib/reporting");
const constants = require("zigbee-herdsman-converters/lib/constants");

/********************************************************************
  This file (`switch_custom.js`) is generated. 
  
  You can edit it for testing, but for PRs please use:
  - `device_db.yaml`                - add or edit devices
  - `switch_custom.md.jinja`        - update the template
  - `make_z2m_custom_converters.py` - update generation script

  Generate with: `make converters`
********************************************************************/

const romasku = {
    switchAction: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { on_off: 0, off_on: 1, toggle_simple: 2, toggle_smart_sync: 3, toggle_smart_opposite: 4 },
            cluster: "genOnOffSwitchCfg",
            attribute: "switchActions", // Enum8
            description: `Select how switch should work:
            - on_off: When switch physically moved to position 1 it always generates ON command, and when moved to position 2 it generates OFF command
            - off_on: Same as on_off, but positions are swapped
            - toggle_simple: Any press of physical switch will TOGGLE the relay and send TOGGLE command to binds
            - toggle_smart_sync: Any press of physical switch will TOGGLE the relay and send corresponding ON/OFF command to keep binds in sync with relay
            - toggle_smart_opposite: Any press of physical switch: TOGGLE the relay and send corresponding ON/OFF command to keep binds in the state opposite to the relay`,
        }),
    switchMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { toggle: 0, momentary: 1, momentary_nc: 2 },
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff00, type: 0x30 }, // Enum8
            description: "Select the type of switch connected to the device",
        }),
    relayMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { detached: 0, press_start: 1, short_press: 3, long_press: 2},
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff01, type: 0x30 }, // Enum8
            description: "When to turn on/off internal relay",
        }),
    relayIndex: (name, endpointName, relay_cnt) =>
        enumLookup({
            name,
            endpointName,
            lookup: Object.fromEntries(
                Array.from({ length: relay_cnt || 2 }, (_, i) => [`relay_${i + 1}`, i + 1])
            ),
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff02, type: 0x20 }, // uint8
            description: "Which internal relay it should trigger",
        }),
    bindedMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { press_start: 1, short_press: 3, long_press: 2},
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff05, type: 0x30 }, // Enum8
            description: "When turn on/off binded device",
        }),
    longPressDuration: (name, endpointName) =>
        numeric({
            name,
            endpointNames: [endpointName],
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff03, type: 0x21 }, // uint16
            description: "What duration is considerd to be long press",
            valueMin: 0,
            valueMax: 5000,
        }),
    levelMoveRate: (name, endpointName) =>
        numeric({
            name,
            endpointNames: [endpointName],
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff04, type: 0x20 }, // uint8
            description: "Level (dim) move rate in steps per ms",
            valueMin: 1,
            valueMax: 255,
        }),
    pressAction: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            access: "STATE_GET",
            lookup: { released: 0, press: 1, long_press: 2, position_on: 3, position_off: 4 },
            cluster: "genMultistateInput",
            attribute: "presentValue",
            description: "Action of the switch: 'released' or 'press' or 'long_press'",
        }),
    relayIndicatorMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { same: 0, opposite: 1, manual: 2 },
            cluster: "genOnOff",
            attribute: { ID: 0xff01, type: 0x30 }, // Enum8
            description: "Mode for the relay indicator LED",
        }),
    relayIndicator: (name, endpointName) =>
        binary({
            name,
            endpointName,
            valueOn: ["ON", 1],
            valueOff: ["OFF", 0],
            cluster: "genOnOff",
            attribute: {ID: 0xff02, type: 0x10},  // Boolean
            description: "State of the relay indicator LED",
            access: "ALL",
        }),
    networkIndicator: (name, endpointName) =>
        binary({
            name,
            endpointName,
            valueOn: ["ON", 1],
            valueOff: ["OFF", 0],
            cluster: "genBasic",
            attribute: {ID: 0xff01, type: 0x10},  // Boolean
            description: "State of the network indicator LED",
            access: "ALL",
        }),
    deviceConfig: (name, endpointName) =>
        text({
            name,
            endpointName,
            access: "ALL",
            cluster: "genBasic",
            attribute:  { ID: 0xff00, type: 0x44 }, // long str
            description: "Current configuration of the device",
            zigbeeCommandOptions: {timeout: 30_000},
            validate: (value) => {
                assertString(value);
                
                const validatePin = (pin) => {
                    const validPins = [
                        "A0", "A1", "A2", "A3", "A4", "A5", "A6","A7",
                        "B0", "B1", "B2", "B3", "B4", "B5", "B6","B7",
                        "C0", "C1", "C2", "C3", "C4", "C5", "C6","C7",
                        "D0", "D1", "D2", "D3", "D4", "D5", "D6","D7",
                        "E0", "E1", "E2", "E3",
                    ];
                    if (!validPins.includes(pin)) throw new Error(`Pin ${pin} is invalid`);
                }

                if (value.length > 256) throw new Error('Length of config is greater than 256');
                if (!value.endsWith(';')) throw new Error('Should end with ;');
                const parts = value.slice(0, -1).split(';');  // Drop last ;
                if (parts.length < 2) throw new Error("Model and/or manufacturer missing");
                for (const part of parts.slice(2)) {
                    if (part == 'SLP') {
                        continue;   
                    } if (part[0] == 'B' || part[0] == 'S') {
                        validatePin(part.slice(1,3));
                        if (!["u", "U", "d", "f"].includes(part[3])) {
                            throw new Error(`Pull up down ${part[3]} is invalid. Valid options are u, U, d, f`);
                        } 
                    } else if (part[0] == 'L' || part[0] == 'R' || part[0] == 'I') {
                        validatePin(part.slice(1,3));
                    } else if(part[0] == 'M') {
                        ;
                    } else if(part[0] == 'i') {
                        ; // TODO: write validation
                    } else {
                        throw new Error(`Invalid entry ${part}. Should start with one of B, R, L, S, I`);
                    }
                }
            }
        }),
};

const definitions = [
    {
        zigbeeModel: [
            "TS0002_HOCUS",
        ],
        model: "SMG 2ch-RF",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_right": 2, "relay_left": 3, "relay_right": 4, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 2),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 2),
            romasku.bindedMode("switch_right_binded_mode", "switch_right"),
            romasku.longPressDuration("switch_right_long_press_duration", "switch_right"),
            romasku.levelMoveRate("switch_right_level_move_rate", "switch_right"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            const endpoint1 = device.getEndpoint(1);
            await reporting.bind(endpoint1, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint1.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint2 = device.getEndpoint(2);
            await reporting.bind(endpoint2, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint2.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
];

module.exports = definitions;
