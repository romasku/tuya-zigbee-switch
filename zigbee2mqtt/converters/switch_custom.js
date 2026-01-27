const {
    numeric,
    enumLookup,
    deviceEndpoints,
    onOff,
    text,
    binary,
    windowCovering,
    deviceAddCustomCluster,
} = require("zigbee-herdsman-converters/lib/modernExtend");
const {assertString} = require("zigbee-herdsman-converters/lib/utils");
const reporting = require("zigbee-herdsman-converters/lib/reporting");
const constants = require("zigbee-herdsman-converters/lib/constants");
const Zcl = require('zigbee-herdsman').Zcl;

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
            attribute: {ID: 0x0010, type: 0x30, required: true, write: true, min: 0, max: 4}, // Enum8
            description: `Select how switch should work:
            - on_off: When switch physically moved to position 1 it always generates ON command, and when moved to position 2 it generates OFF command
            - off_on: Same as on_off, but positions are swapped
            - toggle_simple: Any press of physical switch will TOGGLE the relay and send TOGGLE command to binds
            - toggle_smart_sync: Any press of physical switch will TOGGLE the relay and send corresponding ON/OFF command to keep binds in sync with relay
            - toggle_smart_opposite: Any press of physical switch: TOGGLE the relay and send corresponding ON/OFF command to keep binds in the state opposite to the relay`,
            entityCategory: "config",
        }),
    switchMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { toggle: 0, momentary: 1, momentary_nc: 2 },
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff00, type: 0x30 }, // Enum8
            description: "Select the type of switch connected to the device",
            entityCategory: "config",
        }),
    relayMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { detached: 0, press_start: 1, short_press: 3, long_press: 2},
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff01, type: 0x30 }, // Enum8
            description: "When to turn on/off internal relay",
            entityCategory: "config",
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
            entityCategory: "config",
        }),
    bindedMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { press_start: 1, short_press: 3, long_press: 2},
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff05, type: 0x30 }, // Enum8
            description: "When turn on/off binded device",
            entityCategory: "config",
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
            entityCategory: "config",
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
            entityCategory: "config",
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
            entityCategory: "diagnostic",
        }),
    relayIndicatorMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { same: 0, opposite: 1, manual: 2 },
            cluster: "genOnOff",
            attribute: { ID: 0xff01, type: 0x30 }, // Enum8
            description: "Mode for the relay indicator LED",
            entityCategory: "config",
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
            entityCategory: "config",
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
            entityCategory: "config",
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
                    } else if (part[0] == 'C') {
                        validatePin(part.slice(1,3));
                        validatePin(part.slice(3,5));
                    } else if (part[0] == 'L' || part[0] == 'R' || part[0] == 'I') {
                        validatePin(part.slice(1,3));
                    } else if(part[0] == 'M') {
                        ;
                    } else if(part[0] == 'i') {
                        ; // TODO: write validation
                    } else {
                        throw new Error(`Invalid entry ${part}. Should start with one of B, R, L, S, I, C`);
                    }
                }
            },
            entityCategory: "config",
        }),
    coverMoving: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            access: "STATE_GET",
            lookup: {
                stopped: 0,
                opening: 1,
                closing: 2
            },
            cluster: "closuresWindowCovering",
            attribute: "moving",
            description: "Cover movement status",
            entityCategory: "diagnostic",
        }),
    coverMotorReversal: (name, endpointName) =>
        binary({
            name,
            endpointName,
            valueOn: [true, 1],
            valueOff: [false, 0],
            cluster: "closuresWindowCovering",
            attribute: "motorReversal",
            description: "Reverse motor direction (swap OPEN/CLOSE relays)",
            entityCategory: "config",
        }),
};

const definitions = [
    {
        zigbeeModel: [
            "TS0004-MC",
        ],
        model: "TYWB 4ch-RF",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-MC2",
        ],
        model: "TYWB 4ch-RF",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-SC",
        ],
        model: "ZG-2002-RF",
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
    {
        zigbeeModel: [
            "DEV-ZTU2",
        ],
        model: "Zigbee_SoC_Board_V2_(ZTU)",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "WHD02-Aubess",
            "WHD02-Aubess-ED",
        ],
        model: "WHD02",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-AUB",
        ],
        model: "TMZ02",
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
    {
        zigbeeModel: [
            "TS0003-AUB",
        ],
        model: "TS0003_switch_module_2",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-custom",
        ],
        model: "TS0004_switch_module",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-AVB",
            "TS0001-Avatto-custom",
            "TS0001-AV-CUS",
        ],
        model: "ZWSM16-1-Zigbee",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-AVB",
            "TS0002-Avatto-custom",
            "TS0002-AV-CUS",
        ],
        model: "ZWSM16-2-Zigbee",
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
    {
        zigbeeModel: [
            "TS0003-AVB",
            "TS0003-Avatto-custom",
            "TS0003-AV-CUS",
        ],
        model: "ZWSM16-3-Zigbee",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-AVB",
            "TS0004-Avatto-custom",
            "TS0004-AV-CUS",
        ],
        model: "ZWSM16-4-Zigbee",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0003-AVB2",
        ],
        model: "ZWSM16-3-Zigbee",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-AVB2",
        ],
        model: "ZWSM16-4-Zigbee",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-AV-DRY",
        ],
        model: "TS0001",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0011-avatto",
            "TS0011-avatto-ED",
        ],
        model: "LZWSM16-1",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0012-avatto",
            "TS0012-avatto-ED",
        ],
        model: "LZWSM16-2",
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
    {
        zigbeeModel: [
            "TS0012-AVB1",
        ],
        model: "LZWSM16-2",
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
    {
        zigbeeModel: [
            "TS0013-AVB",
        ],
        model: "LZWSM16-3",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "EKAC-T3092Z-CUSTOM",
        ],
        model: "EKAC-T3092Z",
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
    {
        zigbeeModel: [
            "TS0012-EKF",
        ],
        model: "TS0012",
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
    {
        zigbeeModel: [
            "TS0001-GS",
        ],
        model: "TS0001",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-GS",
        ],
        model: "TS0002",
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
    {
        zigbeeModel: [
            "TS0002-NS",
        ],
        model: "L13Z",
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
    {
        zigbeeModel: [
            "TS0003-GRA",
            "TS0003-GR",
        ],
        model: "TS0003_switch_module_2",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-GIR",
        ],
        model: "JR-ZDS01",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-GIR",
            "TS0002-custom",
        ],
        model: "TS0002_basic",
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
    {
        zigbeeModel: [
            "TS130F-GIR-DUAL",
        ],
        model: "TS130F_GIRIER_DUAL",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceAddCustomCluster("closuresWindowCovering", {
                ID: 0x0102,
                attributes: {
                    moving: {ID: 0xff00, type: Zcl.DataType.ENUM8},
                    motorReversal: {ID: 0xff01, type: Zcl.DataType.BOOLEAN},
                },
            }),
            deviceEndpoints({ endpoints: {"cover_left": 1, "cover_right": 2, } }),
            romasku.deviceConfig("device_config", "cover_left"),
            romasku.networkIndicator("network_led", "cover_left"),
            windowCovering({ 
                controls: ["lift"],
                coverInverted: true,
                configureReporting: false,
                endpointNames: ["cover_left"]
            }),
            romasku.coverMoving("cover_left_moving", "cover_left"),
            romasku.coverMotorReversal("cover_left_motor_reversal", "cover_left"),
            windowCovering({ 
                controls: ["lift"],
                coverInverted: true,
                configureReporting: false,
                endpointNames: ["cover_right"]
            }),
            romasku.coverMoving("cover_right_moving", "cover_right"),
            romasku.coverMotorReversal("cover_right_motor_reversal", "cover_right"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {

            const cover1 = device.getEndpoint(1);
            await reporting.bind(cover1, coordinatorEndpoint, ["closuresWindowCovering"]);
            await cover1.configureReporting("closuresWindowCovering", [
                {
                    attribute: "moving",
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const cover2 = device.getEndpoint(2);
            await reporting.bind(cover2, coordinatorEndpoint, ["closuresWindowCovering"]);
            await cover2.configureReporting("closuresWindowCovering", [
                {
                    attribute: "moving",
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-GIR-1",
        ],
        model: "JR-ZDS01",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-HOB1",
        ],
        model: "WHD02",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-HOB",
        ],
        model: "ZG-301Z",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0011-HOMMYN",
        ],
        model: "TS0011",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-IHS",
        ],
        model: "_TZ3000_pgq7ormg",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0003-IHS",
            "TS0003-3CH-cus",
        ],
        model: "_TZ3000_mhhxxjrs",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-IHS",
        ],
        model: "_TZ3000_knoj8lpk",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-MSB",
        ],
        model: "ZM-104B-M",
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
    {
        zigbeeModel: [
            "TS0003-custom",
        ],
        model: "MS-104CZ",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-MS",
        ],
        model: "ZM4LT2",
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
    {
        zigbeeModel: [
            "TS0003-MS",
        ],
        model: "ZM4LT3",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-MS",
        ],
        model: "ZM4LT4",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "MS105-ZB-CUSTOM",
        ],
        model: "TS0001",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "ZBMINIL2-custom",
        ],
        model: "ZBMINIL2",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-TLED",
        ],
        model: "WHD02",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-C",
        ],
        model: "TS0001",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-C",
        ],
        model: "TS0002_basic_2",
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
    {
        zigbeeModel: [
            "WHD02-custom",
        ],
        model: "WHD02",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "WHD02-custom",
        ],
        model: "WHD02",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "WHD02-custom",
        ],
        model: "WHD02",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0011-custom",
        ],
        model: "TS0011_switch_module",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0011-CUS-2",
        ],
        model: "TS0011_switch_module",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0012-custom",
            "TS0042-CUSTOM",
            "TS0012-custom-end-device",
        ],
        model: "TS0012_switch_module",
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
    {
        zigbeeModel: [
            "ZB08-custom",
            "ZB08-custom-ED",
        ],
        model: "ZB08",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-QS",
        ],
        model: "TS0002_limited",
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
    {
        zigbeeModel: [
            "NovatoZRM01",
        ],
        model: "QS-Zigbee-SEC01-U",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "NovatoZRM02",
        ],
        model: "QS-Zigbee-SEC02-U",
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
    {
        zigbeeModel: [
            "NovatoZNR01",
        ],
        model: "TS0011",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0012-QS",
        ],
        model: "TS0012",
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
    {
        zigbeeModel: [
            "TS130F-NOV",
        ],
        model: "TS130F",
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
    {
        zigbeeModel: [
            "TS0001-custom",
        ],
        model: "TS0001_switch_module",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-OXT-CUS",
        ],
        model: "TS0002_basic",
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
    {
        zigbeeModel: [
            "TS0002-custom",
        ],
        model: "TS0002_basic",
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
    {
        zigbeeModel: [
            "TS011F-TUYA",
        ],
        model: "TS011F_plug_1",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-TS",
        ],
        model: "WHD02",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-Avv",
        ],
        model: "TS0004_switch_module",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-NS1",
        ],
        model: "L13Z",
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
    {
        zigbeeModel: [
            "TS011F-BS-PM",
        ],
        model: "TS011F_plug_1_2",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS011F-BS",
        ],
        model: "_TZ3000_o1jzcxou",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0044-CUS",
        ],
        model: "_TZ3000_mh9px7cq",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-AVT",
        ],
        model: "RoomsAI_37022454",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-AVT",
        ],
        model: "37022463-2",
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
    {
        zigbeeModel: [
            "TS0003-AVT",
            "Avatto-3-touch",
        ],
        model: "370224742",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-AVT",
        ],
        model: "TS0004",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-BSDB",
            "TS0001-BS-T",
        ],
        model: "TS0001",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-BSDB",
            "TS0002-BS-1",
        ],
        model: "TS0002",
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0003-BSDB",
            "TS0003-BSEED",
        ],
        model: "TS0003",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "BSLR1",
        ],
        model: "TS0011",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "BSLR2",
            "Bseed-2-gang-2",
            "Bseed-2-gang-2-ED",
        ],
        model: "TS0012",
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "BSLR3",
            "TS0013-2-BS",
        ],
        model: "TS0013",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
            romasku.bindedMode("switch_right_binded_mode", "switch_right"),
            romasku.longPressDuration("switch_right_long_press_duration", "switch_right"),
            romasku.levelMoveRate("switch_right_level_move_rate", "switch_right"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_middle_indicator_mode", "relay_middle"),
            romasku.relayIndicator("relay_middle_indicator", "relay_middle"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint5.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint6.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-BSMN",
        ],
        model: "TS0001",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0003-BSMN",
            "TS0003-BS",
        ],
        model: "TS0003",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-BSMN",
            "TS0004-BS",
        ],
        model: "TS0004",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-CUS-T",
        ],
        model: "TS0001",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-CUS-T",
        ],
        model: "TS0002",
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0011-BS-T",
        ],
        model: "TS0011",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Bseed-2-gang",
            "Bseed-2-gang-ED",
        ],
        model: "TS0012",
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Bseed-2-gang-3",
        ],
        model: "TS0012",
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0013-BS",
        ],
        model: "TS0013",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
            romasku.bindedMode("switch_right_binded_mode", "switch_right"),
            romasku.longPressDuration("switch_right_long_press_duration", "switch_right"),
            romasku.levelMoveRate("switch_right_level_move_rate", "switch_right"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_middle_indicator_mode", "relay_middle"),
            romasku.relayIndicator("relay_middle_indicator", "relay_middle"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint5.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint6.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0726-1-BS",
        ],
        model: "EC-GL86ZPCS11",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0726-2-BS",
        ],
        model: "EC-GL86ZPCS21",
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0726-3-BS",
        ],
        model: "EC-GL86ZPCS31",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
            romasku.bindedMode("switch_right_binded_mode", "switch_right"),
            romasku.longPressDuration("switch_right_long_press_duration", "switch_right"),
            romasku.levelMoveRate("switch_right_level_move_rate", "switch_right"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_middle_indicator_mode", "relay_middle"),
            romasku.relayIndicator("relay_middle_indicator", "relay_middle"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint5.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint6.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-HMT",
        ],
        model: "Homeetec_37022454",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-HMT",
        ],
        model: "37022463-1",
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
    {
        zigbeeModel: [
            "TS0003-HMT",
        ],
        model: "37022474_1",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            romasku.networkIndicator("network_led", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-IHS-T",
        ],
        model: "_TZ3000_qq9ahj6z",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-IHS-T",
        ],
        model: "_TZ3000_zxrfobzw",
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
    {
        zigbeeModel: [
            "TS0011-MH",
        ],
        model: "TS0011",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0012-MH",
        ],
        model: "TS0012",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_right": 2, "relay_left": 3, "relay_right": 4, } }),
            romasku.deviceConfig("device_config", "switch_left"),
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0013-MH",
        ],
        model: "TS0013",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
            romasku.bindedMode("switch_right_binded_mode", "switch_right"),
            romasku.longPressDuration("switch_right_long_press_duration", "switch_right"),
            romasku.levelMoveRate("switch_right_level_move_rate", "switch_right"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_middle_indicator_mode", "relay_middle"),
            romasku.relayIndicator("relay_middle_indicator", "relay_middle"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint5.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint6.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0011-MHB",
        ],
        model: "TS0011",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0012-MHB",
        ],
        model: "TS0012",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_right": 2, "relay_left": 3, "relay_right": 4, } }),
            romasku.deviceConfig("device_config", "switch_left"),
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0013-MHB",
        ],
        model: "TS0013",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
            romasku.bindedMode("switch_right_binded_mode", "switch_right"),
            romasku.longPressDuration("switch_right_long_press_duration", "switch_right"),
            romasku.levelMoveRate("switch_right_level_move_rate", "switch_right"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_middle_indicator_mode", "relay_middle"),
            romasku.relayIndicator("relay_middle_indicator", "relay_middle"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint5.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint6.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Moes-1-gang",
            "Moes-1-gang-ED",
        ],
        model: "ZS-EUB_1gang",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Moes-2-gang",
            "Moes-2-gang-ED",
        ],
        model: "ZS-EUB_2gang",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_right": 2, "relay_left": 3, "relay_right": 4, } }),
            romasku.deviceConfig("device_config", "switch_left"),
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Moes-3-gang",
            "Moes-3-gang-ED",
        ],
        model: "TS0013",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_middle": 2, "switch_right": 3, "relay_left": 4, "relay_middle": 5, "relay_right": 6, } }),
            romasku.deviceConfig("device_config", "switch_left"),
            onOff({ endpointNames: ["relay_left", "relay_middle", "relay_right"] }),
            romasku.pressAction("switch_left_press_action", "switch_left"),
            romasku.switchMode("switch_left_mode", "switch_left"),
            romasku.switchAction("switch_left_action_mode", "switch_left"),
            romasku.relayMode("switch_left_relay_mode", "switch_left"),
            romasku.relayIndex("switch_left_relay_index", "switch_left", 3),
            romasku.bindedMode("switch_left_binded_mode", "switch_left"),
            romasku.longPressDuration("switch_left_long_press_duration", "switch_left"),
            romasku.levelMoveRate("switch_left_level_move_rate", "switch_left"),
            romasku.pressAction("switch_middle_press_action", "switch_middle"),
            romasku.switchMode("switch_middle_mode", "switch_middle"),
            romasku.switchAction("switch_middle_action_mode", "switch_middle"),
            romasku.relayMode("switch_middle_relay_mode", "switch_middle"),
            romasku.relayIndex("switch_middle_relay_index", "switch_middle", 3),
            romasku.bindedMode("switch_middle_binded_mode", "switch_middle"),
            romasku.longPressDuration("switch_middle_long_press_duration", "switch_middle"),
            romasku.levelMoveRate("switch_middle_level_move_rate", "switch_middle"),
            romasku.pressAction("switch_right_press_action", "switch_right"),
            romasku.switchMode("switch_right_mode", "switch_right"),
            romasku.switchAction("switch_right_action_mode", "switch_right"),
            romasku.relayMode("switch_right_relay_mode", "switch_right"),
            romasku.relayIndex("switch_right_relay_index", "switch_right", 3),
            romasku.bindedMode("switch_right_binded_mode", "switch_right"),
            romasku.longPressDuration("switch_right_long_press_duration", "switch_right"),
            romasku.levelMoveRate("switch_right_level_move_rate", "switch_right"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_middle_indicator_mode", "relay_middle"),
            romasku.relayIndicator("relay_middle_indicator", "relay_middle"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint5.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint6.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "MS4",
        ],
        model: "TS0014",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
            romasku.relayIndicatorMode("relay_0_indicator_mode", "relay_0"),
            romasku.relayIndicator("relay_0_indicator", "relay_0"),
            romasku.relayIndicatorMode("relay_1_indicator_mode", "relay_1"),
            romasku.relayIndicator("relay_1_indicator", "relay_1"),
            romasku.relayIndicatorMode("relay_2_indicator_mode", "relay_2"),
            romasku.relayIndicator("relay_2_indicator", "relay_2"),
            romasku.relayIndicatorMode("relay_3_indicator_mode", "relay_3"),
            romasku.relayIndicator("relay_3_indicator", "relay_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint5.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint6.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint7.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint8.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-PS",
            "T441",
        ],
        model: "T441",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
            romasku.relayIndicatorMode("relay_indicator_mode", "relay"),
            romasku.relayIndicator("relay_indicator", "relay"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

            await endpoint2.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-PS",
            "T442",
        ],
        model: "T442",
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-CUS",
        ],
        model: "TS0001",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch": 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "switch"),
            romasku.networkIndicator("network_led", "switch"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_press_action", "switch"),
            romasku.switchMode("switch_mode", "switch"),
            romasku.switchAction("switch_action_mode", "switch"),
            romasku.relayMode("switch_relay_mode", "switch"),
            romasku.relayIndex("switch_relay_index", "switch", 1),
            romasku.bindedMode("switch_binded_mode", "switch"),
            romasku.longPressDuration("switch_long_press_duration", "switch"),
            romasku.levelMoveRate("switch_level_move_rate", "switch"),
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
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-CUS",
        ],
        model: "TS0004",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_0": 1, "switch_1": 2, "switch_2": 3, "switch_3": 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "switch_0"),
            romasku.networkIndicator("network_led", "switch_0"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_0_press_action", "switch_0"),
            romasku.switchMode("switch_0_mode", "switch_0"),
            romasku.switchAction("switch_0_action_mode", "switch_0"),
            romasku.relayMode("switch_0_relay_mode", "switch_0"),
            romasku.relayIndex("switch_0_relay_index", "switch_0", 4),
            romasku.bindedMode("switch_0_binded_mode", "switch_0"),
            romasku.longPressDuration("switch_0_long_press_duration", "switch_0"),
            romasku.levelMoveRate("switch_0_level_move_rate", "switch_0"),
            romasku.pressAction("switch_1_press_action", "switch_1"),
            romasku.switchMode("switch_1_mode", "switch_1"),
            romasku.switchAction("switch_1_action_mode", "switch_1"),
            romasku.relayMode("switch_1_relay_mode", "switch_1"),
            romasku.relayIndex("switch_1_relay_index", "switch_1", 4),
            romasku.bindedMode("switch_1_binded_mode", "switch_1"),
            romasku.longPressDuration("switch_1_long_press_duration", "switch_1"),
            romasku.levelMoveRate("switch_1_level_move_rate", "switch_1"),
            romasku.pressAction("switch_2_press_action", "switch_2"),
            romasku.switchMode("switch_2_mode", "switch_2"),
            romasku.switchAction("switch_2_action_mode", "switch_2"),
            romasku.relayMode("switch_2_relay_mode", "switch_2"),
            romasku.relayIndex("switch_2_relay_index", "switch_2", 4),
            romasku.bindedMode("switch_2_binded_mode", "switch_2"),
            romasku.longPressDuration("switch_2_long_press_duration", "switch_2"),
            romasku.levelMoveRate("switch_2_level_move_rate", "switch_2"),
            romasku.pressAction("switch_3_press_action", "switch_3"),
            romasku.switchMode("switch_3_mode", "switch_3"),
            romasku.switchAction("switch_3_action_mode", "switch_3"),
            romasku.relayMode("switch_3_relay_mode", "switch_3"),
            romasku.relayIndex("switch_3_relay_index", "switch_3", 4),
            romasku.bindedMode("switch_3_binded_mode", "switch_3"),
            romasku.longPressDuration("switch_3_long_press_duration", "switch_3"),
            romasku.levelMoveRate("switch_3_level_move_rate", "switch_3"),
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
            await reporting.bind(endpoint3, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint3.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint4 = device.getEndpoint(4);
            await reporting.bind(endpoint4, coordinatorEndpoint, ["genMultistateInput"]);
            // switch action:
            await endpoint4.configureReporting("genMultistateInput", [
                {
                    attribute: {ID: 0x0055 /* presentValue */, type: 0x21}, // uint16
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MAX,
                change: 1,
            });

        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Zemi-2-gang",
            "Zemi-2-gang-ED",
        ],
        model: "TS0012",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {"switch_left": 1, "switch_right": 2, "relay_left": 3, "relay_right": 4, } }),
            romasku.deviceConfig("device_config", "switch_left"),
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
            romasku.relayIndicatorMode("relay_left_indicator_mode", "relay_left"),
            romasku.relayIndicator("relay_left_indicator", "relay_left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "relay_right"),
            romasku.relayIndicator("relay_right_indicator", "relay_right"),
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

            await endpoint3.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
            await endpoint4.configureReporting("genOnOff", [
                {
                    attribute: {ID: 0xff02, type: 0x10}, // Boolean
                    minimumReportInterval: 0,
                    maximumReportInterval: constants.repInterval.MAX,
                    reportableChange: 1,
                },
            ]);
        },
        ota: true,
    },
];

module.exports = definitions;
