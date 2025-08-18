import { setupAttributes } from "zigbee-herdsman-converters/lib/modernExtend";
import {
  assertString,
  isDummyDevice,
  postfixWithEndpointName,
} from "zigbee-herdsman-converters/lib/utils";
import exposes from "zigbee-herdsman-converters/lib/exposes";
import { Zcl } from "zigbee-herdsman";
import { logger } from "zigbee-herdsman-converters/lib/logger";

import fz from "zigbee-herdsman-converters/converters/fromZigbee";
import tz from "zigbee-herdsman-converters/converters/toZigbee";

const NS = "zhc:tuya-zigbee-switch";

const e = exposes.presets;
const ea = exposes.access;

const SWITCH_MODE_ENUM = {
  toggle: 0,
  momentary: 1,
  multifunction: 2,
};

const SWITCH_ACTIONS_ENUM = {
  on_off: 0,
  off_on: 1,
  toggle_simple: 2,
  toggle_smart_sync: 3,
  toggle_smart_opposite: 4,
};

const RELAY_MODE_ENUM = {
  detached: 0,
  raise: 1,
  short_press: 3,
  long_press: 2,
};

const BOUND_MODE_ENUM = {
  raise: 1,
  short_press: 3,
  long_press: 2,
};

const PRESS_ACTIONS_ENUM = {
  released: 0,
  press: 1,
  long_press: 2,
};

const RELAY_INDICATOR_MODE_ENUM = {
  same: 0,
  opposite: 1,
  manual: 2,
};

const customExposes = {
  switchMode: (switchIndex) =>
    e
      .enum(`switch_mode`, ea.ALL, Object.keys(SWITCH_MODE_ENUM))
      .withLabel(`Switch ${switchIndex} mode`)
      .withDescription("Select the type of switch connected to the device")
      .withEndpoint(`switch_${switchIndex}`),
  switchActions: (switchIndex) =>
    e
      .enum(`switch_actions`, ea.ALL, Object.keys(SWITCH_ACTIONS_ENUM))
      .withLabel(`Switch ${switchIndex} actions`)
      .withDescription(
        `
             on_off → moving switch to one side always sends ON, to the other 
             side OFF; off_on → same as above but sides swapped; toggle_simple → every press
             toggles the relay and sends TOGGLE; toggle_smart_sync → press toggles relay and 
             sends matching ON/OFF (same as relay); toggle_smart_opposite → press toggles relay
              and sends opposite ON/OFF (opposite to relay).
            `
      )
      // Z2M ui renders this as single paragraph, so code formatted
      // in a same way to represent it as it will look like in Z2M UI
      .withEndpoint(`switch_${switchIndex}`),
  relayMode: (switchIndex) =>
    e
      .enum(`relay_mode`, ea.ALL, Object.keys(RELAY_MODE_ENUM))
      .withLabel(`Switch ${switchIndex} relay mode`)
      .withDescription("When to turn on/off internal relay")
      .withEndpoint(`switch_${switchIndex}`),
  relayIndex: (switchIndex, relayCount) =>
    e
      .enum(
        `relay_index`,
        ea.ALL,
        Array.from({ length: relayCount || 2 }, (_, i) => `relay_${i + 1}`)
      )
      .withLabel(`Switch ${switchIndex} relay index`)
      .withDescription("Which internal relay it should trigger")
      .withEndpoint(`switch_${switchIndex}`),
  boundMode: (switchIndex) =>
    e
      .enum(`bound_mode`, ea.ALL, Object.keys(BOUND_MODE_ENUM))
      .withLabel(`Switch ${switchIndex} bound mode`)
      .withDescription("When to turn on/off bound device")
      .withEndpoint(`switch_${switchIndex}`),
  longPressDuration: (switchIndex) =>
    e
      .numeric(`long_press_duration`, ea.ALL)
      .withValueMin(0)
      .withValueMax(5000)
      .withLabel(`Switch ${switchIndex} long press duration`)
      .withDescription("What duration is considered to be long press")
      .withEndpoint(`switch_${switchIndex}`),
  levelMoveRate: (switchIndex) =>
    e
      .numeric(`level_move_rate`, ea.ALL)
      .withValueMin(1)
      .withValueMax(255)
      .withLabel(`Switch ${switchIndex} level move rate`)
      .withDescription("Level (dim) move rate in steps per ms")
      .withEndpoint(`switch_${switchIndex}`),
  pressAction: (switchIndex) =>
    e
      .enum(`press_action`, ea.STATE_GET, Object.keys(PRESS_ACTIONS_ENUM))
      .withLabel(`Switch ${switchIndex} action`)
      .withDescription(
        "Action of the switch: 'released' or 'press' or 'long_press'"
      )
      .withEndpoint(`switch_${switchIndex}`),
  networkIndicator: () =>
    e
      .binary(`network_led`, ea.ALL, "ON", "OFF")
      .withDescription("State of the network indicator LED")
      .withEndpoint("main"),
  relayIndicatorMode: (relayIndex) =>
    e
      .enum(
        "relay_indicator_mode",
        ea.ALL,
        Object.keys(RELAY_INDICATOR_MODE_ENUM)
      )
      .withLabel(`Relay ${relayIndex} action`)
      .withDescription("Mode for the relay indicator LED")
      .withEndpoint(`relay_${relayIndex}`),
  relayIndicator: (relayIndex) =>
    e
      .binary(`relay_indicator`, ea.ALL, "ON", "OFF")
      .withDescription("State of the relay indicator LED")
      .withEndpoint(`relay_${relayIndex}`),
  deviceConfig: () =>
    e
      .text("device_config", ea.ALL)
      .withLabel("Current configuration of the device")
      .withEndpoint("main"),
};

const validateConfig = (value) => {
  assertString(value);

  const PIN_RE = /^(?:[ABCD][0-7]|E[0-3])$/; // A-D:0-7, E:0-3
  const validatePin = (pin) => {
    if (!PIN_RE.test(pin)) throw new Error(`Pin ${pin} is invalid`);
  };

  if (value.length > 256)
    throw new Error("Length of config is greater than 256");
  if (!value.endsWith(";")) throw new Error("Should end with ;");
  const parts = value.slice(0, -1).split(";"); // Drop last ;
  if (parts.length < 2) throw new Error("Model and/or manufacturer missing");
  for (const partRaw of parts.slice(2)) {
    const part = partRaw.trim();
    if (!part) continue;
    const tag = part[0];
    if (tag === "B" || tag === "S") {
      if (part.length < 4) throw new Error(`Entry ${part} is too short`);
      validatePin(part.slice(1, 3));
      const mode = part[3];
      if (!["u", "U", "d", "f"].includes(mode)) {
        throw new Error(`Pull mode '${mode}' invalid (use u, U, d, f)`);
      }
    } else if (tag === "L" || tag === "R" || tag === "I") {
      if (part.length < 3) throw new Error(`Entry ${part} missing pin`);
      validatePin(part.slice(1, 3));
    } else if (tag === "M" || tag === "i") {
      // TODO: validation for these
    } else {
      throw new Error(
        `Invalid entry ${part}. Should start with one of B, R, L, S, I, M, i`
      );
    }
  }
};

const customToZigbee = {
  switchMode: {
    key: ["switch_mode"],
    convertSet: async (entity, key, value, meta) => {
      const enc = SWITCH_MODE_ENUM[value];
      if (enc === undefined) throw new Error(`Invalid switch mode: ${value}`);
      await entity.write("genOnOffSwitchCfg", { switchMode: enc });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOffSwitchCfg", ["switchMode"]);
    },
  },
  switchActions: {
    key: ["switch_actions"],
    convertSet: async (entity, key, value, meta) => {
      const enc = SWITCH_ACTIONS_ENUM[value];
      if (enc === undefined) throw new Error(`Invalid switch action: ${value}`);
      await entity.write("genOnOffSwitchCfg", { switchActions: enc });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOffSwitchCfg", ["switchActions"]);
    },
  },
  relayMode: {
    key: ["relay_mode"],
    convertSet: async (entity, key, value, meta) => {
      const enc = RELAY_MODE_ENUM[value];
      if (enc === undefined) throw new Error(`Invalid relay mode: ${value}`);
      await entity.write("genOnOffSwitchCfg", { relayMode: enc });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOffSwitchCfg", ["relayMode"]);
    },
  },
  relayIndex: {
    key: ["relay_index"],
    convertSet: async (entity, key, value, meta) => {
      const idx = Number(String(value).replace(/^relay_/, ""));
      if (!Number.isInteger(idx) || idx < 1)
        throw new Error(`Invalid relay index: ${value}`);
      await entity.write("genOnOffSwitchCfg", { relayIndex: idx });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOffSwitchCfg", ["relayIndex"]);
    },
  },
  longPressDuration: {
    key: ["long_press_duration"],
    convertSet: async (entity, key, value, meta) => {
      const enc = Number(value);
      if (!Number.isFinite(enc) || enc < 0 || enc > 5000) {
        throw new Error(`Invalid long press duration: ${value}`);
      }
      await entity.write("genOnOffSwitchCfg", { longPressDuration: enc });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOffSwitchCfg", ["longPressDuration"]);
    },
  },
  levelMoveRate: {
    key: ["level_move_rate"],
    convertSet: async (entity, key, value, meta) => {
      const enc = Number(value);
      if (!Number.isFinite(enc) || enc < 1 || enc > 255) {
        throw new Error(`Invalid level move rate: ${value}`);
      }
      await entity.write("genOnOffSwitchCfg", { levelMoveRate: enc });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOffSwitchCfg", ["levelMoveRate"]);
    },
  },
  boundMode: {
    key: ["bound_mode"],
    convertSet: async (entity, key, value, meta) => {
      const enc = BOUND_MODE_ENUM[value];
      if (enc === undefined) throw new Error(`Invalid bound mode: ${value}`);
      await entity.write("genOnOffSwitchCfg", { boundMode: enc });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOffSwitchCfg", ["boundMode"]);
    },
  },
  pressAction: {
    key: ["press_action"],
    convertGet: async (entity, key, meta) => {
      await entity.read("genMultistateInput", ["presentValue"]);
    },
  },
  networkLed: {
    key: ["network_led"],
    convertSet: async (entity, key, value, meta) => {
      const on = (v) =>
        typeof v === "string" ? v.toUpperCase() === "ON" : !!v;
      await entity.write("genBasic", { networkLed: on(value) ? 1 : 0 });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genBasic", ["networkLed"]);
    },
  },
  relayIndicatorMode: {
    key: ["relay_indicator_mode"],
    convertSet: async (entity, key, value, meta) => {
      const enc = RELAY_INDICATOR_MODE_ENUM[value];
      if (enc === undefined)
        throw new Error(`Invalid relay indicator mode: ${value}`);
      await entity.write("genOnOff", { relayIndicatorMode: enc });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOff", ["relayIndicatorMode"]);
    },
  },
  relayIndicator: {
    key: ["relay_indicator"],
    convertSet: async (entity, key, value, meta) => {
      const on = (v) =>
        typeof v === "string" ? v.toUpperCase() === "ON" : !!v;
      await entity.write("genOnOff", { relayIndicator: on(value) ? 1 : 0 });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genOnOff", ["relayIndicator"]);
    },
  },
  deviceConfig: {
    key: ["device_config"],
    convertSet: async (entity, key, value, meta) => {
      validateConfig(value);
      await entity.write("genBasic", { deviceConfig: value });
      return { state: { [key]: value } };
    },
    convertGet: async (entity, key, meta) => {
      await entity.read("genBasic", ["deviceConfig"]);
    },
  },
};

const invert = (obj) =>
  Object.fromEntries(Object.entries(obj).map(([k, v]) => [v, k]));
const DEC = {
  SWITCH_MODE: invert(SWITCH_MODE_ENUM),
  SWITCH_ACTIONS: invert(SWITCH_ACTIONS_ENUM),
  RELAY_MODE: invert(RELAY_MODE_ENUM),
  BOUND_MODE: invert(BOUND_MODE_ENUM),
  PRESS_ACTIONS: invert(PRESS_ACTIONS_ENUM),
  RELAY_INDICATOR_MODE: invert(RELAY_INDICATOR_MODE_ENUM),
};

const customFromZigbee = {
  switchConfigAttrs: {
    cluster: "genOnOffSwitchCfg",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      const result = {};
      if (msg.data.switchMode !== undefined) {
        result[postfixWithEndpointName("switch_mode", msg, model, meta)] =
          DEC.SWITCH_MODE[msg.data.switchMode] ?? msg.data.switchMode;
      }
      if (msg.data.switchActions !== undefined) {
        result[postfixWithEndpointName("switch_actions", msg, model, meta)] =
          DEC.SWITCH_ACTIONS[msg.data.switchActions] ?? msg.data.switchActions;
      }
      if (msg.data.relayMode !== undefined) {
        result[postfixWithEndpointName("relay_mode", msg, model, meta)] =
          DEC.RELAY_MODE[msg.data.relayMode] ?? msg.data.relayMode;
      }
      if (msg.data.relayIndex !== undefined) {
        result[
          postfixWithEndpointName("relay_index", msg, model, meta)
        ] = `relay_${msg.data.relayIndex}`;
      }
      if (msg.data.longPressDuration !== undefined) {
        result[
          postfixWithEndpointName("long_press_duration", msg, model, meta)
        ] = msg.data.longPressDuration;
      }
      if (msg.data.levelMoveRate !== undefined) {
        result[postfixWithEndpointName("level_move_rate", msg, model, meta)] =
          msg.data.levelMoveRate;
      }
      if (msg.data.boundMode !== undefined) {
        result[postfixWithEndpointName("bound_mode", msg, model, meta)] =
          DEC.BOUND_MODE[msg.data.boundMode] ?? msg.data.boundMode;
      }
      logger.debug(
        `customFromZigbee.switchConfigAttrs ${JSON.stringify(result)}`,
        NS
      );
      return result;
    },
  },
  pressAction: {
    cluster: "genMultistateInput",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      const result = {};
      if (msg.data.presentValue !== undefined) {
        result[postfixWithEndpointName("press_action", msg, model, meta)] =
          DEC.PRESS_ACTIONS[msg.data.presentValue] ?? msg.data.presentValue;
      }
      logger.debug(
        `customFromZigbee.pressAction ${JSON.stringify(result)}`,
        NS
      );
      return result;
    },
  },
  networkLed: {
    cluster: "genBasic",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      const result = {};
      if (msg.data.networkLed !== undefined) {
        result["network_led_main"] = msg.data.networkLed === 1 ? "ON" : "OFF";
      }
      logger.debug(`customFromZigbee.networkLed ${JSON.stringify(result)}`, NS);
      return result;
    },
  },
  relayIndicatorMode: {
    cluster: "genOnOff",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      const result = {};
      if (msg.data.relayIndicatorMode !== undefined) {
        result[
          postfixWithEndpointName("relay_indicator_mode", msg, model, meta)
        ] =
          DEC.RELAY_INDICATOR_MODE[msg.data.relayIndicatorMode] ??
          msg.data.relayIndicatorMode;
      }
      logger.debug(
        `customFromZigbee.relayIndicatorMode ${JSON.stringify(result)}`,
        NS
      );
      return result;
    },
  },
  relayIndicator: {
    cluster: "genOnOff",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      const result = {};
      if (msg.data.relayIndicator !== undefined) {
        result[postfixWithEndpointName("relay_indicator", msg, model, meta)] =
          msg.data.relayIndicator === 1 ? "ON" : "OFF";
      }
      logger.debug(
        `customFromZigbee.relayIndicator ${JSON.stringify(result)}`,
        NS
      );
      return result;
    },
  },
  deviceConfig: {
    cluster: "genBasic",
    type: ["readResponse"],
    convert(model, msg, publish, options, meta) {
      const result = {};
      if (msg.data.deviceConfig !== undefined) {
        result["device_config_main"] = msg.data.deviceConfig;
      }
      logger.debug(
        `customFromZigbee.deviceConfig ${JSON.stringify(result)}`,
        NS
      );
      return result;
    },
  },
};

const parseConfig = (config) => {
  const parts = config.slice(0, -1).split(";"); // Drop last ;
  let relayCount = 0;
  let relayIndicatorCnt = 0;
  let switchCount = 0;
  let hasNetworkLed = false;

  for (const part of parts.slice(2)) {
    if (part[0] === "S") {
      switchCount++;
    } else if (part[0] === "R") {
      relayCount++;
    } else if (part[0] === "L") {
      hasNetworkLed = true;
    } else if (part[0] === "I") {
      relayIndicatorCnt++;
    }
  }

  if (relayIndicatorCnt > relayCount) {
    logger.warning(
      `There are more relay indicators (${relayIndicatorCnt}) than relays (${relayCount})`,
      NS
    );
  }

  return {
    relayCount,
    switchCount,
    relayIndicatorCnt,
    hasNetworkLed,
  };
};

const CUSTOM_CLUSTERS = {
  genBasic: {
    ID: Zcl.Clusters.genBasic.ID,
    attributes: {
      deviceConfig: { ID: 0xff00, type: Zcl.DataType.LONG_CHAR_STR },
      networkLed: { ID: 0xff01, type: Zcl.DataType.BOOLEAN },
    },
    commands: {},
    commandsResponse: {},
  },
  genOnOff: {
    ID: Zcl.Clusters.genOnOff.ID,
    attributes: {
      relayIndicatorMode: { ID: 0xff01, type: Zcl.DataType.ENUM8 },
      relayIndicator: { ID: 0xff02, type: Zcl.DataType.BOOLEAN },
    },
    commands: {},
    commandsResponse: {},
  },
  genOnOffSwitchCfg: {
    ID: Zcl.Clusters.genOnOffSwitchCfg.ID,
    attributes: {
      switchMode: { ID: 0xff00, type: Zcl.DataType.ENUM8 }, // toggle/momentary/multifunction
      relayMode: { ID: 0xff01, type: Zcl.DataType.ENUM8 }, // detached/raise/long_press/short_press
      relayIndex: { ID: 0xff02, type: Zcl.DataType.UINT8 }, // 1..N
      longPressDuration: { ID: 0xff03, type: Zcl.DataType.UINT16 }, // ms
      levelMoveRate: { ID: 0xff04, type: Zcl.DataType.UINT8 }, // steps/ms
      boundMode: { ID: 0xff05, type: Zcl.DataType.ENUM8 }, // raise/long_press/short_press
    },
    commands: {},
    commandsResponse: {},
  },
};

const registerCustomClusters = (device) => {
  for (const [clusterName, clusterDefinition] of Object.entries(
    CUSTOM_CLUSTERS
  )) {
    if (clusterDefinition === undefined) {
      continue;
    }
    if (!device.customClusters[clusterName]) {
      device.addCustomCluster(clusterName, clusterDefinition);
    }
  }
};

const baseDefinition = {
  vendor: "Tuya-custom",
  description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
  exposes: (device, options) => {
    const dynExposes = [];
    let config = !isDummyDevice(device)
      ? device
        .getEndpoint(1)
        .getClusterAttributeValue("genBasic", "deviceConfig")
      : null;

    if (!config) {
      return [customExposes.deviceConfig()];
    }

    const deviceConfig = parseConfig(config);

    for (let i = 1; i <= deviceConfig.relayCount; i++) {
      const switchExpose = e.switch(``).withEndpoint(`relay_${i}`);
      // .withLabel cannot be used for this internal feature, so we need the following hack:
      const state = switchExpose.features.find(f => f.name === 'state');
      if (state) state.label = `Relay ${i} state`;
      dynExposes.push(switchExpose);
      dynExposes.push(
        e
          .power_on_behavior(["off", "on", "toggle", "previous"])
          .withLabel(`Relay ${i} Power-on behavior`)
          .withEndpoint(`relay_${i}`)
      );
    }
    for (let i = 1; i <= deviceConfig.relayIndicatorCnt; i++) {
      dynExposes.push(
        customExposes.relayIndicator(i),
        customExposes.relayIndicatorMode(i)
      );
    }
    for (let i = 1; i <= deviceConfig.switchCount; i++) {
      dynExposes.push(
        customExposes.pressAction(i),
        customExposes.switchMode(i),
        customExposes.switchActions(i),
        customExposes.relayMode(i),
        customExposes.relayIndex(i, deviceConfig.relayCount),
        customExposes.boundMode(i),
        customExposes.longPressDuration(i),
        customExposes.levelMoveRate(i)
      );
    }

    if (deviceConfig.hasNetworkLed) {
      dynExposes.push(customExposes.networkIndicator());
    }
    dynExposes.push(customExposes.deviceConfig());

    return dynExposes;
  },
  endpoint: (device) => {
    let config = !isDummyDevice(device)
      ? device
        .getEndpoint(1)
        .getClusterAttributeValue("genBasic", "deviceConfig")
      : null;

    if (!config) {
      return [];
    }

    const deviceConfig = parseConfig(config);

    const res = {};
    for (let i = 0; i < deviceConfig.switchCount; i++) {
      res[`switch_${i + 1}`] = i + 1;
    }
    for (let i = 0; i < deviceConfig.relayCount; i++) {
      res[`relay_${i + 1}`] = deviceConfig.switchCount + i + 1;
    }
    res["main"] = 1;
    return res;
  },

  toZigbee: [
    tz.on_off,
    tz.power_on_behavior,
    ...Object.values(customToZigbee),
  ],
  fromZigbee: [
    fz.on_off,
    fz.power_on_behavior,
    ...Object.values(customFromZigbee),
  ],

  extend: [],
  meta: { multiEndpoint: true },
  onEvent: (event, _, device) => {
    // There is change in API here, so this code checks for both cases
    if (event === "start") {
      registerCustomClusters(device);
    }
    if (event.type === "start") {
      registerCustomClusters(event.data.device);
    }
  },
  configure: async (device, coordinatorEndpoint, definition) => {
    registerCustomClusters(device);

    definition.model = "some_model";
    definition.icon = "http://google.com";

    const mainEp = device.getEndpoint(1);
    await mainEp.read("genBasic", ["deviceConfig"]);

    let config = mainEp.getClusterAttributeValue("genBasic", "deviceConfig");

    logger.debug(`Read config ${config} during configure`, NS);

    const deviceConfig = parseConfig(config);

    logger.debug(
      `Parsed config as ${JSON.stringify(deviceConfig)} during configure`,
      NS
    );

    if (deviceConfig.hasNetworkLed) {
      await setupAttributes(
        mainEp,
        coordinatorEndpoint,
        "genBasic",
        [{ attribute: "networkLed", min: -1, max: -1, change: -1 }],
        false
      );
    }

    for (let i = 1; i <= deviceConfig.relayCount; i++) {
      const relayEp = device.getEndpoint(deviceConfig.switchCount + i);
      await setupAttributes(relayEp, coordinatorEndpoint, "genOnOff", [
        { min: 0, max: 60, change: 1, attribute: "onOff" },
      ]);
      await setupAttributes(
        relayEp,
        coordinatorEndpoint,
        "genOnOff",
        [{ attribute: "startUpOnOff", min: -1, max: -1, change: -1 }],
        false
      );
    }
    for (let i = 1; i <= deviceConfig.relayIndicatorCnt; i++) {
      const indEp = device.getEndpoint(deviceConfig.switchCount + i);
      await setupAttributes(
        indEp,
        coordinatorEndpoint,
        "genOnOff",
        [
          { attribute: "relayIndicator", min: -1, max: -1, change: -1 },
          { attribute: "relayIndicatorMode", min: -1, max: -1, change: -1 },
        ],
        false
      );
    }
    for (let i = 1; i <= deviceConfig.switchCount; i++) {
      const switchEp = device.getEndpoint(i);
      await setupAttributes(
        switchEp,
        coordinatorEndpoint,
        "genOnOffSwitchCfg",
        [
          { attribute: "switchActions", min: -1, max: -1, change: -1 },
          { attribute: "switchMode", min: -1, max: -1, change: -1 },
          { attribute: "relayMode", min: -1, max: -1, change: -1 },
          { attribute: "relayIndex", min: -1, max: -1, change: -1 },
          { attribute: "longPressDuration", min: -1, max: -1, change: -1 },
          { attribute: "levelMoveRate", min: -1, max: -1, change: -1 },
          { attribute: "boundMode", min: -1, max: -1, change: -1 },
        ],
        false
      );
      await setupAttributes(
        switchEp,
        coordinatorEndpoint,
        "genMultistateInput",
        [{ min: 0, max: 60, change: 1, attribute: "presentValue" }]
      );
    }
  },
  ota: true,
}

const MODELS = [ 
    {
        zigbeeModel: [
            "TS0012-custom",
            "TS0042-CUSTOM",
        ],
        model: "TS0012_switch_module",
    },
    {
        zigbeeModel: [
            "TS0012-custom-end-device",
            "TS0042-CUSTOM",
        ],
        model: "TS0012_switch_module",
    },
    {
        zigbeeModel: [
            "WHD02-custom",
        ],
        model: "WHD02",
    },
    {
        zigbeeModel: [
            "TS0002-custom",
        ],
        model: "TS0002_basic",
    },
    {
        zigbeeModel: [
            "TS0002-OXT-CUS",
        ],
        model: "TS0002_basic",
    },
    {
        zigbeeModel: [
            "TS0011-custom",
        ],
        model: "TS0011_switch_module",
    },
    {
        zigbeeModel: [
            "TS0011-custom",
        ],
        model: "TS0011_switch_module",
    },
    {
        zigbeeModel: [
            "TS0001-custom",
        ],
        model: "TS0001_switch_module",
    },
    {
        zigbeeModel: [
            "TS0002-custom",
        ],
        model: "TS0002_basic",
    },
    {
        zigbeeModel: [
            "TS0001-AVB",
            "TS0001-Avatto-custom",
            "TS0001-AV-CUS",
        ],
        model: "TS0001_switch_module",
    },
    {
        zigbeeModel: [
            "TS0002-AVB",
            "TS0002-Avatto-custom",
            "TS0002-AV-CUS",
        ],
        model: "TS0002_limited",
    },
    {
        zigbeeModel: [
            "TS0003-AVB",
            "TS0003-Avatto-custom",
            "TS0003-AV-CUS",
        ],
        model: "TS0003_switch_module_2",
    },
    {
        zigbeeModel: [
            "TS0004-AVB",
            "TS0004-Avatto-custom",
            "TS0004-AV-CUS",
        ],
        model: "TS0004_switch_module_2",
    },
    {
        zigbeeModel: [
            "Moes-2-gang",
        ],
        model: "TS0012",
    },
    {
        zigbeeModel: [
            "Moes-2-gang-ED",
        ],
        model: "TS0012",
    },
    {
        zigbeeModel: [
            "Bseed-2-gang",
        ],
        model: "TS0012",
    },
    {
        zigbeeModel: [
            "Bseed-2-gang-ED",
        ],
        model: "TS0012",
    },
    {
        zigbeeModel: [
            "Bseed-2-gang-2",
        ],
        model: "TS0012",
    },
    {
        zigbeeModel: [
            "Bseed-2-gang-2-ED",
        ],
        model: "TS0012",
    },
    {
        zigbeeModel: [
            "TS0012-avatto",
        ],
        model: "TS0012_switch_module",
    },
    {
        zigbeeModel: [
            "TS0012-avatto-ED",
        ],
        model: "TS0012_switch_module",
    },
    {
        zigbeeModel: [
            "WHD02-Aubess",
        ],
        model: "WHD02",
    },
    {
        zigbeeModel: [
            "WHD02-Aubess-ED",
        ],
        model: "WHD02",
    },
    {
        zigbeeModel: [
            "Moes-1-gang",
        ],
        model: "ZS-EUB_1gang",
    },
    {
        zigbeeModel: [
            "Moes-1-gang-ED",
        ],
        model: "ZS-EUB_1gang",
    },
    {
        zigbeeModel: [
            "Moes-3-gang",
        ],
        model: "TS0013",
    },
    {
        zigbeeModel: [
            "Moes-3-gang-ED",
        ],
        model: "TS0013",
    },
    {
        zigbeeModel: [
            "WHD02-custom",
        ],
        model: "WHD02",
    },
    {
        zigbeeModel: [
            "WHD02-custom",
        ],
        model: "WHD02",
    },
    {
        zigbeeModel: [
            "Zemi-2-gang",
        ],
        model: "TS0012",
    },
    {
        zigbeeModel: [
            "Zemi-2-gang-ED",
        ],
        model: "TS0012",
    },
    {
        zigbeeModel: [
            "TS0011-avatto",
        ],
        model: "LZWSM16-1",
    },
    {
        zigbeeModel: [
            "TS0011-avatto-ED",
        ],
        model: "LZWSM16-1",
    },
    {
        zigbeeModel: [
            "TS0003-custom",
        ],
        model: "TS0003",
    },
    {
        zigbeeModel: [
            "TS0003-IHS",
            "TS0003-3CH-cus",
        ],
        model: "TS0003_switch_module_2",
    },
    {
        zigbeeModel: [
            "TS0004-IHS",
        ],
        model: "TS0004_switch_module_2",
    },
    {
        zigbeeModel: [
            "ZB08-custom",
        ],
        model: "TS0013_switch_module",
    },
    {
        zigbeeModel: [
            "ZB08-custom-ED",
        ],
        model: "TS0013_switch_module",
    },
    {
        zigbeeModel: [
            "TS0004-Avv",
        ],
        model: "TS0004_switch_module",
    },
    {
        zigbeeModel: [
            "TS0004-custom",
        ],
        model: "TS0004_switch_module",
    },
    {
        zigbeeModel: [
            "Avatto-3-touch",
        ],
        model: "TS0003_switch_3_gang",
    },
]

const definitions = MODELS.map((model) => ({...model, ...baseDefinition}));

export default definitions;
