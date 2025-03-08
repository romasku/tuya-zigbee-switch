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

const romasku = {
    switchAction: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { on_off: 0, off_on: 1, toggle: 2 },
            cluster: "genOnOffSwitchCfg",
            attribute: "switchActions", // Enum8
            description: `Select how switch should work:
            - on_off: When switch physically moved to position 1 it always generates ON command, and when moved to position 2 it generates OFF command
            - off_on: Same as on_off, but positions are swapped
            - toggle: Generate TOGGLE command for any press of physicall switch`,
        }),
    switchMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { toggle: 0, momentary: 1, multifunction: 2 },
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff00, type: 0x30 }, // Enum8
            description: "Select the type of switch connected to the device",
        }),
    relayMode: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            lookup: { detached: 0, raise: 1, long_press: 2 },
            cluster: "genOnOffSwitchCfg",
            attribute: { ID: 0xff01, type: 0x30 }, // Enum8
            description: "When to trigger internal relay",
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
    pressAction: (name, endpointName) =>
        enumLookup({
            name,
            endpointName,
            access: "STATE_GET",
            lookup: { released: 0, press: 1, long_press: 2 },
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
            description: "Mode for relay indicator LED",
        }),
    relayIndicator: (name, endpointName) =>
        binary({
            name,
            endpointName,
            valueOn: ["ON", 1],
            valueOff: ["OFF", 0],
            cluster: "genOnOff",
            attribute: {ID: 0xff02, type: 0x10},  // Boolean
            description: "State of relay indicator LED",
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
                    if (part[0] == 'B' || part[0] == 'S') {
                        validatePin(part.slice(1,3));
                        if (!["u", "U", "d", "f"].includes(part[3])) {
                            throw new Error(`Pull up down ${part[3]} is invalid. Valid options are u, U, d, f`);
                        } 
                    } else if (part[0] == 'L' || part[0] == 'R' || part[0] == 'I') {
                        validatePin(part.slice(1,3));
                    } else if(part[0] == 'M') {
                        ;
                    } else if(part[0] == 'i') {
                        ; // TODO: write validatetion
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
            "TS0012-custom",
            "TS0042-CUSTOM",
        ],
        model: "TS0012-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0012-custom-end-device",
            "TS0042-CUSTOM",
        ],
        model: "TS0012-custom-end-device",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "WHD02-custom",
        ],
        model: "WHD02-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 1),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint2 = device.getEndpoint(2);
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-custom",
        ],
        model: "TS0002-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0011-custom",
        ],
        model: "TS0011-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 1),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint2 = device.getEndpoint(2);
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0011-custom",
        ],
        model: "TS0011-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 1),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint2 = device.getEndpoint(2);
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-custom",
        ],
        model: "TS0001-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 1),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint2 = device.getEndpoint(2);
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-custom",
        ],
        model: "TS0002-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0001-Avatto-custom",
        ],
        model: "TS0001-Avatto-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 1),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint2 = device.getEndpoint(2);
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0002-Avatto-custom",
        ],
        model: "TS0002-Avatto-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0004-AV-CUS",
            "TS0004-Avatto-custom",
        ],
        model: "TS0004-Avatto-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, 3: 3, 4: 4, "relay_0": 5, "relay_1": 6, "relay_2": 7, "relay_3": 8, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["relay_0", "relay_1", "relay_2", "relay_3"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 4),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 4),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
            romasku.pressAction("switch_3_press_action", "3"),
            romasku.switchMode("switch_3_mode", "3"),
            romasku.switchAction("switch_3_action_mode", "3"),
            romasku.relayMode("switch_3_relay_mode", "3"),
            romasku.relayIndex("switch_3_relay_index", "3", 4),
            romasku.longPressDuration("switch_3_long_press_duration", "3"),
            romasku.pressAction("switch_4_press_action", "4"),
            romasku.switchMode("switch_4_mode", "4"),
            romasku.switchAction("switch_4_action_mode", "4"),
            romasku.relayMode("switch_4_relay_mode", "4"),
            romasku.relayIndex("switch_4_relay_index", "4", 4),
            romasku.longPressDuration("switch_4_long_press_duration", "4"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(3), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(4), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint5 = device.getEndpoint(5);
            await reporting.onOff(endpoint5, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint6 = device.getEndpoint(6);
            await reporting.onOff(endpoint6, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint7 = device.getEndpoint(7);
            await reporting.onOff(endpoint7, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint8 = device.getEndpoint(8);
            await reporting.onOff(endpoint8, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Moes-2-gang",
        ],
        model: "Moes-2-gang",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "left"),
            romasku.relayIndicator("relay_left_indicator", "left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "right"),
            romasku.relayIndicator("relay_right_indicator", "right"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Moes-2-gang-ED",
        ],
        model: "Moes-2-gang",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "left"),
            romasku.relayIndicator("relay_left_indicator", "left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "right"),
            romasku.relayIndicator("relay_right_indicator", "right"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "Bseed-2-gang-ED",
        ],
        model: "Bseed-2-gang",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
            romasku.relayIndicatorMode("relay_left_indicator_mode", "left"),
            romasku.relayIndicator("relay_left_indicator", "left"),
            romasku.relayIndicatorMode("relay_right_indicator_mode", "right"),
            romasku.relayIndicator("relay_right_indicator", "right"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0012-avatto",
        ],
        model: "TS0012-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "TS0012-avatto-ED",
        ],
        model: "TS0012-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, 2: 2, "left": 3, "right": 4, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["left", "right"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 2),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
            romasku.pressAction("switch_2_press_action", "2"),
            romasku.switchMode("switch_2_mode", "2"),
            romasku.switchAction("switch_2_action_mode", "2"),
            romasku.relayMode("switch_2_relay_mode", "2"),
            romasku.relayIndex("switch_2_relay_index", "2", 2),
            romasku.longPressDuration("switch_2_long_press_duration", "2"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            await reporting.bind(device.getEndpoint(2), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint3 = device.getEndpoint(3);
            await reporting.onOff(endpoint3, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
            const endpoint4 = device.getEndpoint(4);
            await reporting.onOff(endpoint4, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "WHD02-Aubess",
        ],
        model: "WHD02-AUBESS-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 1),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint2 = device.getEndpoint(2);
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
    {
        zigbeeModel: [
            "WHD02-Aubess-ED",
        ],
        model: "WHD02-AUBESS-custom",
        vendor: "Tuya-custom",
        description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
        extend: [
            deviceEndpoints({ endpoints: {1: 1, "relay": 2, } }),
            romasku.deviceConfig("device_config", "1"),
            onOff({ endpointNames: ["relay"] }),
            romasku.pressAction("switch_1_press_action", "1"),
            romasku.switchMode("switch_1_mode", "1"),
            romasku.switchAction("switch_1_action_mode", "1"),
            romasku.relayMode("switch_1_relay_mode", "1"),
            romasku.relayIndex("switch_1_relay_index", "1", 1),
            romasku.longPressDuration("switch_1_long_press_duration", "1"),
        ],
        meta: { multiEndpoint: true },
        configure: async (device, coordinatorEndpoint, logger) => {
            await reporting.bind(device.getEndpoint(1), coordinatorEndpoint, ["genMultistateInput"]);
            const endpoint2 = device.getEndpoint(2);
            await reporting.onOff(endpoint2, {
                min: 0,
                max: constants.repInterval.MINUTE,
                change: 1,
            });
        },
        ota: true,
    },
];

module.exports = definitions;
