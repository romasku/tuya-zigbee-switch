import { setupAttributes } from "zigbee-herdsman-converters/lib/modernExtend";
import {
  assertString,
  getOptions,
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
  momentary_nc: 2,
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
  press_start: 1,
  long_press: 2,
  short_press: 3,
};

const BOUND_MODE_ENUM = {
  press_start: 1,
  long_press: 2,
  short_press: 3,
};

const PRESS_ACTIONS_ENUM = {
  released: 0,
  press: 1,
  long_press: 2,
  position_on: 3,
  position_off: 4,
};

const RELAY_INDICATOR_MODE_ENUM = {
  same: 0,
  opposite: 1,
  manual: 2,
};

const COVER_SWITCH_PRESS_ACTIONS_ENUM = {
  released: 0,
  open: 1,
  close: 2,
  stop: 3,
  long_open: 4,
  long_close: 5,
};

const COVER_SWITCH_TYPE_ENUM = {
  toggle: 0,
  momentary: 1,
};

const COVER_SWITCH_MODE_ENUM = {
  immediate: 0,
  short_press: 1,
  long_press: 2,
  hybrid: 3,
};

const COVER_MOVING_ENUM = {
  stopped: 0,
  opening: 1,
  closing: 2,
};

const MAIN_ENDPOINT = "main";

const invertLookup = (obj) =>
  Object.fromEntries(Object.entries(obj).map(([key, value]) => [value, key]));

const DEC = {
  SWITCH_MODE: invertLookup(SWITCH_MODE_ENUM),
  SWITCH_ACTIONS: invertLookup(SWITCH_ACTIONS_ENUM),
  RELAY_MODE: invertLookup(RELAY_MODE_ENUM),
  BOUND_MODE: invertLookup(BOUND_MODE_ENUM),
  PRESS_ACTIONS: invertLookup(PRESS_ACTIONS_ENUM),
  RELAY_INDICATOR_MODE: invertLookup(RELAY_INDICATOR_MODE_ENUM),
  COVER_SWITCH_PRESS_ACTIONS: invertLookup(COVER_SWITCH_PRESS_ACTIONS_ENUM),
  COVER_SWITCH_TYPE: invertLookup(COVER_SWITCH_TYPE_ENUM),
  COVER_SWITCH_MODE: invertLookup(COVER_SWITCH_MODE_ENUM),
  COVER_MOVING: invertLookup(COVER_MOVING_ENUM),
};

const parseNumberInRange = (value, name, min, max) => {
  const parsed = Number(value);
  if (!Number.isInteger(parsed) || parsed < min || parsed > max) {
    throw new Error(`Invalid ${name}: ${value}`);
  }

  return parsed;
};

const toBoolean = (value) => {
  if (typeof value === "string") {
    const normalized = value.trim().toUpperCase();
    if (["ON", "TRUE", "1"].includes(normalized)) {
      return true;
    }
    if (["OFF", "FALSE", "0"].includes(normalized)) {
      return false;
    }
  }

  return !!value;
};

const makeEnumConverter = (key, cluster, attribute, lookup) => ({
  key: [key],
  convertSet: async (entity, exposedKey, value) => {
    const encoded = lookup[String(value)];
    if (encoded === undefined) {
      throw new Error(`Invalid ${key}: ${value}`);
    }

    await entity.write(cluster, { [attribute]: encoded });
    return { state: { [exposedKey]: value } };
  },
  convertGet: async (entity) => {
    await entity.read(cluster, [attribute]);
  },
});

const makeNumericConverter = (key, cluster, attribute, min, max) => ({
  key: [key],
  convertSet: async (entity, exposedKey, value) => {
    const encoded = parseNumberInRange(value, key, min, max);
    await entity.write(cluster, { [attribute]: encoded });
    return { state: { [exposedKey]: encoded } };
  },
  convertGet: async (entity) => {
    await entity.read(cluster, [attribute]);
  },
});

const makeBinaryConverter = (key, cluster, attribute) => ({
  key: [key],
  convertSet: async (entity, exposedKey, value) => {
    const encoded = toBoolean(value);
    await entity.write(cluster, { [attribute]: encoded ? 1 : 0 });
    return {
      state: {
        [exposedKey]: typeof value === "boolean" ? value : encoded ? "ON" : "OFF",
      },
    };
  },
  convertGet: async (entity) => {
    await entity.read(cluster, [attribute]);
  },
});

const getDeviceConfigString = (device) =>
  device?.getEndpoint?.(1)?.getClusterAttributeValue("genBasic", "deviceConfig");

const parseConfig = (config) => {
  const result = {
    switchCount: 0,
    relayCount: 0,
    relayIndicatorCount: 0,
    coverSwitchCount: 0,
    coverCount: 0,
    hasNetworkLed: false,
    hasBatteryCluster: false,
  };

  const parts = config.slice(0, -1).split(";");
  for (const rawPart of parts.slice(2)) {
    const part = rawPart.trim();
    if (!part) {
      continue;
    }

    if (part === "SLP" || part === "M" || part[0] === "D" || part[0] === "i") {
      continue;
    }

    if (part.startsWith("BT")) {
      result.hasBatteryCluster = true;
      continue;
    }

    if (part[0] === "S") {
      result.switchCount += 1;
    } else if (part[0] === "R") {
      result.relayCount += 1;
    } else if (part[0] === "I") {
      result.relayIndicatorCount += 1;
    } else if (part[0] === "L") {
      result.hasNetworkLed = true;
    } else if (part[0] === "X") {
      result.coverSwitchCount += 1;
    } else if (part[0] === "C") {
      result.coverCount += 1;
    }
  }

  if (result.relayIndicatorCount > result.relayCount) {
    logger.warning(
      `There are more relay indicators (${result.relayIndicatorCount}) than relays (${result.relayCount})`,
      NS
    );
  }

  return result;
};

const getEndpointRole = (deviceConfig, endpointId) => {
  if (endpointId >= 1 && endpointId <= deviceConfig.switchCount) {
    return { type: "switch", index: endpointId };
  }

  const relayBase = deviceConfig.switchCount + 1;
  const relayMax = relayBase + deviceConfig.relayCount - 1;
  if (endpointId >= relayBase && endpointId <= relayMax) {
    return {
      type: "relay",
      index: endpointId - relayBase + 1,
    };
  }

  const coverSwitchBase = relayMax + 1;
  const coverSwitchMax = coverSwitchBase + deviceConfig.coverSwitchCount - 1;
  if (endpointId >= coverSwitchBase && endpointId <= coverSwitchMax) {
    return {
      type: "cover_switch",
      index: endpointId - coverSwitchBase + 1,
    };
  }

  const coverBase = coverSwitchMax + 1;
  const coverMax = coverBase + deviceConfig.coverCount - 1;
  if (endpointId >= coverBase && endpointId <= coverMax) {
    return {
      type: "cover",
      index: endpointId - coverBase + 1,
    };
  }

  return { type: "unknown", index: endpointId };
};

const getEndpointMap = (deviceConfig) => {
  const endpoints = { [MAIN_ENDPOINT]: 1 };

  for (let i = 1; i <= deviceConfig.switchCount; i++) {
    endpoints[`switch_${i}`] = i;
  }

  for (let i = 1; i <= deviceConfig.relayCount; i++) {
    endpoints[`relay_${i}`] = deviceConfig.switchCount + i;
  }

  for (let i = 1; i <= deviceConfig.coverSwitchCount; i++) {
    endpoints[`cover_switch_${i}`] =
      deviceConfig.switchCount + deviceConfig.relayCount + i;
  }

  for (let i = 1; i <= deviceConfig.coverCount; i++) {
    endpoints[`cover_${i}`] =
      deviceConfig.switchCount +
      deviceConfig.relayCount +
      deviceConfig.coverSwitchCount +
      i;
  }

  return endpoints;
};

const isDummyDevice = (device) =>
  device === undefined || device === null || "isDummyDevice" in device;

const customExposes = {
  batteryPercentage: () =>
    e
      .numeric("battery", ea.STATE_GET)
      .withUnit("%")
      .withValueMin(0)
      .withValueMax(100)
      .withLabel("Battery")
      .withDescription("Remaining battery in %")
      .withCategory("diagnostic"),
  switchMode: (index) =>
    e
      .enum("switch_mode", ea.ALL, Object.keys(SWITCH_MODE_ENUM))
      .withLabel(`Switch ${index} mode`)
      .withDescription("Select the type of switch connected to the device")
      .withEndpoint(`switch_${index}`)
      .withCategory("config"),
  switchActions: (index) =>
    e
      .enum("switch_actions", ea.ALL, Object.keys(SWITCH_ACTIONS_ENUM))
      .withLabel(`Switch ${index} actions`)
      .withDescription(
        "Select how the switch toggles the relay and which commands it sends to bindings"
      )
      .withEndpoint(`switch_${index}`)
      .withCategory("config"),
  relayMode: (index) =>
    e
      .enum("relay_mode", ea.ALL, Object.keys(RELAY_MODE_ENUM))
      .withLabel(`Switch ${index} relay mode`)
      .withDescription("When to turn on or off the internal relay")
      .withEndpoint(`switch_${index}`)
      .withCategory("config"),
  relayIndex: (index, relayCount) =>
    e
      .enum(
        "relay_index",
        ea.ALL,
        Array.from({ length: relayCount || 2 }, (_, i) => `relay_${i + 1}`)
      )
      .withLabel(`Switch ${index} relay index`)
      .withDescription("Which internal relay this switch should control")
      .withEndpoint(`switch_${index}`)
      .withCategory("config"),
  boundMode: (index) =>
    e
      .enum("bound_mode", ea.ALL, Object.keys(BOUND_MODE_ENUM))
      .withLabel(`Switch ${index} bound mode`)
      .withDescription("When to send ON or OFF commands to bound devices")
      .withEndpoint(`switch_${index}`)
      .withCategory("config"),
  longPressDuration: (index) =>
    e
      .numeric("long_press_duration", ea.ALL)
      .withValueMin(0)
      .withValueMax(5000)
      .withLabel(`Switch ${index} long press duration`)
      .withDescription("Threshold in milliseconds for long press detection")
      .withEndpoint(`switch_${index}`)
      .withCategory("config"),
  levelMoveRate: (index) =>
    e
      .numeric("level_move_rate", ea.ALL)
      .withValueMin(1)
      .withValueMax(255)
      .withLabel(`Switch ${index} level move rate`)
      .withDescription("Level move rate in steps per millisecond")
      .withEndpoint(`switch_${index}`)
      .withCategory("config"),
  pressAction: (index) =>
    e
      .enum("press_action", ea.STATE_GET, Object.keys(PRESS_ACTIONS_ENUM))
      .withLabel(`Switch ${index} action`)
      .withDescription("Last physical action reported by the switch")
      .withEndpoint(`switch_${index}`)
      .withCategory("diagnostic"),
  networkIndicator: () =>
    e
      .binary("network_led", ea.ALL, "ON", "OFF")
      .withLabel("Network LED")
      .withDescription("State of the dedicated network indicator LED")
      .withEndpoint(MAIN_ENDPOINT)
      .withCategory("config"),
  relayIndicatorMode: (index) =>
    e
      .enum("relay_indicator_mode", ea.ALL, Object.keys(RELAY_INDICATOR_MODE_ENUM))
      .withLabel(`Relay ${index} indicator mode`)
      .withDescription("How the relay indicator LED behaves")
      .withEndpoint(`relay_${index}`)
      .withCategory("config"),
  relayIndicator: (index) =>
    e
      .binary("relay_indicator", ea.ALL, "ON", "OFF")
      .withLabel(`Relay ${index} indicator`)
      .withDescription("State of the relay indicator LED")
      .withEndpoint(`relay_${index}`)
      .withCategory("config"),
  multiPressResetCount: () =>
    e
      .numeric("multi_press_reset_count", ea.ALL)
      .withValueMin(0)
      .withValueMax(255)
      .withLabel("Multi-press reset count")
      .withDescription(
        "Number of consecutive presses that trigger factory reset; 0 disables it"
      )
      .withEndpoint(MAIN_ENDPOINT)
      .withCategory("config"),
  deviceConfigText: () =>
    e
      .text("device_config", ea.ALL)
      .withLabel("Device config")
      .withDescription("Current configuration string of the device")
      .withEndpoint(MAIN_ENDPOINT)
      .withCategory("config"),
  deviceConfigComposite: () =>
    e
      .composite("device_config", "device_config", ea.ALL)
      .withFeature(customExposes.deviceConfigText())
      .withCategory("config"),
  cover: (index) => e.cover().withEndpoint(`cover_${index}`),
  coverMoving: (index) =>
    e
      .enum("moving", ea.STATE_GET, Object.keys(COVER_MOVING_ENUM))
      .withLabel(`Cover ${index} movement`)
      .withDescription("Current movement status of the cover")
      .withEndpoint(`cover_${index}`)
      .withCategory("diagnostic"),
  coverMotorReversal: (index) =>
    e
      .binary("motor_reversal", ea.ALL, true, false)
      .withLabel(`Cover ${index} motor reversal`)
      .withDescription("Reverse motor direction by swapping OPEN and CLOSE")
      .withEndpoint(`cover_${index}`)
      .withCategory("config"),
  coverSwitchPressAction: (index) =>
    e
      .enum(
        "cover_switch_press_action",
        ea.STATE_GET,
        Object.keys(COVER_SWITCH_PRESS_ACTIONS_ENUM)
      )
      .withLabel(`Cover switch ${index} action`)
      .withDescription("Last physical action reported by the cover switch")
      .withEndpoint(`cover_switch_${index}`)
      .withCategory("diagnostic"),
  coverSwitchType: (index) =>
    e
      .enum("cover_switch_type", ea.ALL, Object.keys(COVER_SWITCH_TYPE_ENUM))
      .withLabel(`Cover switch ${index} type`)
      .withDescription("Toggle uses rocker inputs, momentary uses push buttons")
      .withEndpoint(`cover_switch_${index}`)
      .withCategory("config"),
  coverSwitchCoverIndex: (index, coverCount) =>
    e
      .enum(
        "cover_switch_cover_index",
        ea.ALL,
        ["detached", ...Array.from({ length: coverCount || 2 }, (_, i) => `cover_${i + 1}`)]
      )
      .withLabel(`Cover switch ${index} cover index`)
      .withDescription("Which local cover this switch should control")
      .withEndpoint(`cover_switch_${index}`)
      .withCategory("config"),
  coverSwitchInvert: (index) =>
    e
      .binary("cover_switch_invert", ea.ALL, "ON", "OFF")
      .withLabel(`Cover switch ${index} invert`)
      .withDescription("Invert OPEN and CLOSE directions for the inputs")
      .withEndpoint(`cover_switch_${index}`)
      .withCategory("config"),
  coverSwitchLocalMode: (index) =>
    e
      .enum("cover_switch_local_mode", ea.ALL, Object.keys(COVER_SWITCH_MODE_ENUM))
      .withLabel(`Cover switch ${index} local mode`)
      .withDescription("When to control the local cover from a momentary switch")
      .withEndpoint(`cover_switch_${index}`)
      .withCategory("config"),
  coverSwitchBoundMode: (index) =>
    e
      .enum("cover_switch_bound_mode", ea.ALL, Object.keys(COVER_SWITCH_MODE_ENUM))
      .withLabel(`Cover switch ${index} bound mode`)
      .withDescription("When to send cover commands to bound devices")
      .withEndpoint(`cover_switch_${index}`)
      .withCategory("config"),
  coverSwitchLongPressDuration: (index) =>
    e
      .numeric("cover_switch_long_press_duration", ea.ALL)
      .withValueMin(0)
      .withValueMax(5000)
      .withLabel(`Cover switch ${index} long press duration`)
      .withDescription("Threshold in milliseconds for long press detection")
      .withEndpoint(`cover_switch_${index}`)
      .withCategory("config"),
};

const validateConfig = (value) => {
  assertString(value);

  const PIN_RE = /^(?:[ABCD][0-7])$/;
  const validatePin = (pin) => {
    if (!PIN_RE.test(pin)) {
      throw new Error(`Pin ${pin} is invalid`);
    }
  };

  if (value.length > 256) {
    throw new Error("Length of config is greater than 256");
  }
  if (!value.endsWith(";")) {
    throw new Error("Should end with ;");
  }

  const parts = value.slice(0, -1).split(";");
  if (parts.length < 2) {
    throw new Error("Model and/or manufacturer missing");
  }

  for (const rawPart of parts.slice(2)) {
    const part = rawPart.trim();
    if (!part || part === "SLP" || part === "M") {
      continue;
    }

    if (part[0] === "D") {
      if (!/^D\d+$/.test(part)) {
        throw new Error(`Debounce option ${part} is invalid. Use D<N>, e.g. D100 or D0`);
      }
      continue;
    }

    if (part.startsWith("BT")) {
      validatePin(part.slice(2, 4));
      continue;
    }

    if (part[0] === "B" || part[0] === "S") {
      validatePin(part.slice(1, 3));
      if (!["u", "U", "d", "f"].includes(part[3])) {
        throw new Error(`Pull mode '${part[3]}' is invalid. Use u, U, d or f`);
      }
      continue;
    }

    if (part[0] === "X") {
      validatePin(part.slice(1, 3));
      validatePin(part.slice(3, 5));
      if (!["u", "U", "d", "f"].includes(part[5])) {
        throw new Error(`Pull mode '${part[5]}' is invalid. Use u, U, d or f`);
      }
      continue;
    }

    if (part[0] === "C") {
      validatePin(part.slice(1, 3));
      validatePin(part.slice(3, 5));
      continue;
    }

    if (part[0] === "L" || part[0] === "R" || part[0] === "I") {
      validatePin(part.slice(1, 3));
      continue;
    }

    if (part[0] === "i") {
      continue;
    }

    throw new Error(
      `Invalid entry ${part}. Should start with one of B, BT, C, D, I, L, M, R, S, SLP, X, i`
    );
  }
};

const customToZigbee = {
  state: {
    key: ["state"],
    convertSet: async (entity, key, value, meta) => {
      const endpointName = meta.endpoint_name ?? "";
      const normalized = String(value).toLowerCase();

      if (endpointName.startsWith("cover_")) {
        const lookup = {
          open: "upOpen",
          close: "downClose",
          stop: "stop",
          on: "upOpen",
          off: "downClose",
        };
        const command = lookup[normalized];
        if (command === undefined) {
          throw new Error(`Invalid cover state: ${value}`);
        }

        await entity.command(
          "closuresWindowCovering",
          command,
          {},
          getOptions(meta.mapped, entity)
        );
        return {
          state: {
            [key]:
              normalized === "on"
                ? "OPEN"
                : normalized === "off"
                  ? "CLOSE"
                  : normalized.toUpperCase(),
          },
        };
      }

      if (!["toggle", "off", "on"].includes(normalized)) {
        throw new Error(`Invalid state: ${value}`);
      }

      await entity.command("genOnOff", normalized, {}, getOptions(meta.mapped, entity));
      if (normalized === "toggle") {
        const suffix = meta.endpoint_name ? `_${meta.endpoint_name}` : "";
        const currentState = meta.state[`state${suffix}`];
        return currentState
          ? {
              state: {
                [key]: currentState === "OFF" ? "ON" : "OFF",
              },
            }
          : {};
      }

      return { state: { [key]: normalized.toUpperCase() } };
    },
    convertGet: async (entity, key, meta) => {
      if ((meta.endpoint_name ?? "").startsWith("cover_")) {
        await entity.read("closuresWindowCovering", ["moving"]);
      } else {
        await entity.read("genOnOff", ["onOff"]);
      }
    },
  },
  switchMode: makeEnumConverter(
    "switch_mode",
    "genOnOffSwitchCfg",
    "switchMode",
    SWITCH_MODE_ENUM
  ),
  switchActions: makeEnumConverter(
    "switch_actions",
    "genOnOffSwitchCfg",
    "switchActions",
    SWITCH_ACTIONS_ENUM
  ),
  relayMode: makeEnumConverter(
    "relay_mode",
    "genOnOffSwitchCfg",
    "relayMode",
    RELAY_MODE_ENUM
  ),
  relayIndex: {
    key: ["relay_index"],
    convertSet: async (entity, key, value) => {
      const relayIndex = parseNumberInRange(
        String(value).replace(/^relay_/, ""),
        "relay_index",
        1,
        255
      );
      await entity.write("genOnOffSwitchCfg", { relayIndex });
      return { state: { [key]: `relay_${relayIndex}` } };
    },
    convertGet: async (entity) => {
      await entity.read("genOnOffSwitchCfg", ["relayIndex"]);
    },
  },
  boundMode: makeEnumConverter(
    "bound_mode",
    "genOnOffSwitchCfg",
    "boundMode",
    BOUND_MODE_ENUM
  ),
  longPressDuration: makeNumericConverter(
    "long_press_duration",
    "genOnOffSwitchCfg",
    "longPressDuration",
    0,
    5000
  ),
  levelMoveRate: makeNumericConverter(
    "level_move_rate",
    "genOnOffSwitchCfg",
    "levelMoveRate",
    1,
    255
  ),
  pressAction: {
    key: ["press_action"],
    convertGet: async (entity) => {
      await entity.read("genMultistateInput", ["presentValue"]);
    },
  },
  networkLed: makeBinaryConverter("network_led", "genBasic", "networkLed"),
  relayIndicatorMode: makeEnumConverter(
    "relay_indicator_mode",
    "genOnOff",
    "relayIndicatorMode",
    RELAY_INDICATOR_MODE_ENUM
  ),
  relayIndicator: makeBinaryConverter(
    "relay_indicator",
    "genOnOff",
    "relayIndicator"
  ),
  deviceConfig: {
    key: ["device_config"],
    convertSet: async (entity, key, value) => {
      const config = value.device_config_main;
      validateConfig(config);
      await entity.write("genBasic", { deviceConfig: config });
      return { state: { [key]: { device_config_main: config } } };
    },
    convertGet: async (entity) => {
      await entity.read("genBasic", ["deviceConfig"]);
    },
  },
  multiPressResetCount: makeNumericConverter(
    "multi_press_reset_count",
    "genBasic",
    "multiPressResetCount",
    0,
    255
  ),
  coverSwitchType: makeEnumConverter(
    "cover_switch_type",
    "manuSpecificTuyaCoverSwitchConfig",
    "switchType",
    COVER_SWITCH_TYPE_ENUM
  ),
  coverSwitchCoverIndex: {
    key: ["cover_switch_cover_index"],
    convertSet: async (entity, key, value) => {
      const normalized = String(value);
      const encoded =
        normalized === "detached"
          ? 0
          : parseNumberInRange(
              normalized.replace(/^cover_/, ""),
              "cover_switch_cover_index",
              1,
              255
            );
      await entity.write("manuSpecificTuyaCoverSwitchConfig", {
        coverIndex: encoded,
      });
      return {
        state: {
          [key]: encoded === 0 ? "detached" : `cover_${encoded}`,
        },
      };
    },
    convertGet: async (entity) => {
      await entity.read("manuSpecificTuyaCoverSwitchConfig", ["coverIndex"]);
    },
  },
  coverSwitchInvert: makeBinaryConverter(
    "cover_switch_invert",
    "manuSpecificTuyaCoverSwitchConfig",
    "reversal"
  ),
  coverSwitchLocalMode: makeEnumConverter(
    "cover_switch_local_mode",
    "manuSpecificTuyaCoverSwitchConfig",
    "localMode",
    COVER_SWITCH_MODE_ENUM
  ),
  coverSwitchBoundMode: makeEnumConverter(
    "cover_switch_bound_mode",
    "manuSpecificTuyaCoverSwitchConfig",
    "bindedMode",
    COVER_SWITCH_MODE_ENUM
  ),
  coverSwitchLongPressDuration: makeNumericConverter(
    "cover_switch_long_press_duration",
    "manuSpecificTuyaCoverSwitchConfig",
    "longPressDuration",
    0,
    5000
  ),
  coverSwitchPressAction: {
    key: ["cover_switch_press_action"],
    convertGet: async (entity) => {
      await entity.read("genMultistateInput", ["presentValue"]);
    },
  },
  coverMotorReversal: makeBinaryConverter(
    "motor_reversal",
    "closuresWindowCovering",
    "motorReversal"
  ),
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
        result[postfixWithEndpointName("relay_index", msg, model, meta)] =
          `relay_${msg.data.relayIndex}`;
      }
      if (msg.data.longPressDuration !== undefined) {
        result[postfixWithEndpointName("long_press_duration", msg, model, meta)] =
          msg.data.longPressDuration;
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
  multistateInput: {
    cluster: "genMultistateInput",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      if (msg.data.presentValue === undefined) {
        return {};
      }

      const config = getDeviceConfigString(meta.device);
      if (!config) {
        return {};
      }

      const deviceConfig = parseConfig(config);
      const role = getEndpointRole(deviceConfig, msg.endpoint.ID);
      const result = {};

      if (role.type === "switch") {
        result[postfixWithEndpointName("press_action", msg, model, meta)] =
          DEC.PRESS_ACTIONS[msg.data.presentValue] ?? msg.data.presentValue;
      } else if (role.type === "cover_switch") {
        result[
          postfixWithEndpointName("cover_switch_press_action", msg, model, meta)
        ] =
          DEC.COVER_SWITCH_PRESS_ACTIONS[msg.data.presentValue] ??
          msg.data.presentValue;
      }

      logger.debug(
        `customFromZigbee.multistateInput ${JSON.stringify(result)}`,
        NS
      );
      return result;
    },
  },
  basicAttrs: {
    cluster: "genBasic",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      const result = {};

      if (msg.data.networkLed !== undefined) {
        result.network_led_main = msg.data.networkLed === 1 ? "ON" : "OFF";
      }
      if (msg.data.multiPressResetCount !== undefined) {
        result.multi_press_reset_count_main = msg.data.multiPressResetCount;
      }
      if (msg.data.deviceConfig !== undefined) {
        result.device_config = {
          device_config_main: msg.data.deviceConfig,
        };
      }

      logger.debug(`customFromZigbee.basicAttrs ${JSON.stringify(result)}`, NS);
      return result;
    },
  },
  batteryPercentage: {
    cluster: "genPowerCfg",
    type: ["readResponse", "attributeReport"],
    convert(model, msg) {
      if (msg.data.batteryPercentageRemaining === undefined) {
        return {};
      }

      if (msg.data.batteryPercentageRemaining === 0xff) {
        return {};
      }

      return {
        battery: Math.round(msg.data.batteryPercentageRemaining / 2),
      };
    },
  },
  relayIndicatorAttrs: {
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
      if (msg.data.relayIndicator !== undefined) {
        result[postfixWithEndpointName("relay_indicator", msg, model, meta)] =
          msg.data.relayIndicator === 1 ? "ON" : "OFF";
      }

      logger.debug(
        `customFromZigbee.relayIndicatorAttrs ${JSON.stringify(result)}`,
        NS
      );
      return result;
    },
  },
  coverSwitchConfigAttrs: {
    cluster: "manuSpecificTuyaCoverSwitchConfig",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      const result = {};

      if (msg.data.switchType !== undefined) {
        result[postfixWithEndpointName("cover_switch_type", msg, model, meta)] =
          DEC.COVER_SWITCH_TYPE[msg.data.switchType] ?? msg.data.switchType;
      }
      if (msg.data.coverIndex !== undefined) {
        result[
          postfixWithEndpointName("cover_switch_cover_index", msg, model, meta)
        ] = msg.data.coverIndex === 0 ? "detached" : `cover_${msg.data.coverIndex}`;
      }
      if (msg.data.reversal !== undefined) {
        result[
          postfixWithEndpointName("cover_switch_invert", msg, model, meta)
        ] = msg.data.reversal === 1 ? "ON" : "OFF";
      }
      if (msg.data.localMode !== undefined) {
        result[
          postfixWithEndpointName("cover_switch_local_mode", msg, model, meta)
        ] =
          DEC.COVER_SWITCH_MODE[msg.data.localMode] ?? msg.data.localMode;
      }
      if (msg.data.bindedMode !== undefined) {
        result[
          postfixWithEndpointName("cover_switch_bound_mode", msg, model, meta)
        ] =
          DEC.COVER_SWITCH_MODE[msg.data.bindedMode] ?? msg.data.bindedMode;
      }
      if (msg.data.longPressDuration !== undefined) {
        result[
          postfixWithEndpointName(
            "cover_switch_long_press_duration",
            msg,
            model,
            meta
          )
        ] = msg.data.longPressDuration;
      }

      logger.debug(
        `customFromZigbee.coverSwitchConfigAttrs ${JSON.stringify(result)}`,
        NS
      );
      return result;
    },
  },
  coverAttrs: {
    cluster: "closuresWindowCovering",
    type: ["readResponse", "attributeReport"],
    convert(model, msg, publish, options, meta) {
      const result = {};

      if (msg.data.moving !== undefined) {
        const moving =
          DEC.COVER_MOVING[msg.data.moving] ?? msg.data.moving;
        result[postfixWithEndpointName("moving", msg, model, meta)] = moving;
        if (moving === "opening") {
          result[postfixWithEndpointName("state", msg, model, meta)] = "OPEN";
        } else if (moving === "closing") {
          result[postfixWithEndpointName("state", msg, model, meta)] = "CLOSE";
        } else if (moving === "stopped") {
          result[postfixWithEndpointName("state", msg, model, meta)] = "STOP";
        }
      }
      if (msg.data.motorReversal !== undefined) {
        result[postfixWithEndpointName("motor_reversal", msg, model, meta)] =
          msg.data.motorReversal === 1;
      }

      logger.debug(`customFromZigbee.coverAttrs ${JSON.stringify(result)}`, NS);
      return result;
    },
  },
};

const CUSTOM_CLUSTERS = {
  genBasic: {
    ID: Zcl.Clusters.genBasic.ID,
    attributes: {
      deviceConfig: { ID: 0xff00, type: Zcl.DataType.LONG_CHAR_STR },
      networkLed: { ID: 0xff01, type: Zcl.DataType.BOOLEAN },
      multiPressResetCount: { ID: 0xff02, type: Zcl.DataType.UINT8 },
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
      switchMode: { ID: 0xff00, type: Zcl.DataType.ENUM8 },
      relayMode: { ID: 0xff01, type: Zcl.DataType.ENUM8 },
      relayIndex: { ID: 0xff02, type: Zcl.DataType.UINT8 },
      longPressDuration: { ID: 0xff03, type: Zcl.DataType.UINT16 },
      levelMoveRate: { ID: 0xff04, type: Zcl.DataType.UINT8 },
      boundMode: { ID: 0xff05, type: Zcl.DataType.ENUM8 },
      switchActions: { ID: 0x0010, type: Zcl.DataType.ENUM8 },
    },
    commands: {},
    commandsResponse: {},
  },
  closuresWindowCovering: {
    ID: Zcl.Clusters.closuresWindowCovering.ID,
    attributes: {
      moving: { ID: 0xff00, type: Zcl.DataType.ENUM8 },
      motorReversal: { ID: 0xff01, type: Zcl.DataType.BOOLEAN, write: true },
    },
    commands: {},
    commandsResponse: {},
  },
  manuSpecificTuyaCoverSwitchConfig: {
    ID: 0xfc01,
    manufacturerCode: 0x125d,
    attributes: {
      switchType: { ID: 0x0000, type: Zcl.DataType.ENUM8, write: true },
      coverIndex: { ID: 0x0001, type: Zcl.DataType.UINT8, write: true },
      reversal: { ID: 0x0002, type: Zcl.DataType.BOOLEAN, write: true },
      localMode: { ID: 0x0003, type: Zcl.DataType.ENUM8, write: true },
      bindedMode: { ID: 0x0004, type: Zcl.DataType.ENUM8, write: true },
      longPressDuration: { ID: 0x0005, type: Zcl.DataType.UINT16, write: true },
    },
    commands: {},
    commandsResponse: {},
  },
};

const registerCustomClusters = (device) => {
  for (const [clusterName, clusterDefinition] of Object.entries(CUSTOM_CLUSTERS)) {
    if (!device.customClusters[clusterName]) {
      device.addCustomCluster(clusterName, clusterDefinition);
    }
  }
};

const baseDefinition = {
  vendor: "Tuya-custom",
  description: "Custom switch (https://github.com/romasku/tuya-zigbee-switch)",
  exposes: (device) => {
    const dynamicExposes = [
      customExposes.multiPressResetCount(),
      customExposes.deviceConfigComposite(),
    ];

    if (isDummyDevice(device)) {
      return dynamicExposes;
    }

    const config = getDeviceConfigString(device);
    if (!config) {
      return dynamicExposes;
    }

    const deviceConfig = parseConfig(config);

    if (deviceConfig.hasBatteryCluster) {
      dynamicExposes.unshift(customExposes.batteryPercentage());
    }

    for (let i = 1; i <= deviceConfig.relayCount; i++) {
      dynamicExposes.push(
        e.switch("").withEndpoint(`relay_${i}`),
        e
          .power_on_behavior(["off", "on", "toggle", "previous"])
          .withEndpoint(`relay_${i}`)
      );
    }

    for (let i = 1; i <= deviceConfig.switchCount; i++) {
      dynamicExposes.push(
        customExposes.pressAction(i),
        customExposes.switchMode(i),
        customExposes.switchActions(i),
        customExposes.boundMode(i),
        customExposes.longPressDuration(i),
        customExposes.levelMoveRate(i)
      );

      if (deviceConfig.relayCount > 0) {
        dynamicExposes.push(
          customExposes.relayMode(i),
          customExposes.relayIndex(i, deviceConfig.relayCount)
        );
      }
    }

    for (let i = 1; i <= deviceConfig.relayIndicatorCount; i++) {
      dynamicExposes.push(
        customExposes.relayIndicator(i),
        customExposes.relayIndicatorMode(i)
      );
    }

    for (let i = 1; i <= deviceConfig.coverCount; i++) {
      dynamicExposes.push(
        customExposes.cover(i),
        customExposes.coverMoving(i),
        customExposes.coverMotorReversal(i)
      );
    }

    for (let i = 1; i <= deviceConfig.coverSwitchCount; i++) {
      dynamicExposes.push(
        customExposes.coverSwitchPressAction(i),
        customExposes.coverSwitchType(i),
        customExposes.coverSwitchInvert(i),
        customExposes.coverSwitchBoundMode(i),
        customExposes.coverSwitchLongPressDuration(i)
      );

      if (deviceConfig.coverCount > 0) {
        dynamicExposes.push(
          customExposes.coverSwitchCoverIndex(i, deviceConfig.coverCount),
          customExposes.coverSwitchLocalMode(i)
        );
      }
    }

    if (deviceConfig.hasNetworkLed) {
      dynamicExposes.push(customExposes.networkIndicator());
    }

    return dynamicExposes;
  },
  endpoint: (device) => {
    if (isDummyDevice(device)) {
      return { [MAIN_ENDPOINT]: 1 };
    }

    const config = getDeviceConfigString(device);
    if (!config) {
      return { [MAIN_ENDPOINT]: 1 };
    }

    return getEndpointMap(parseConfig(config));
  },
  toZigbee: [
    customToZigbee.state,
    tz.power_on_behavior,
    customToZigbee.switchMode,
    customToZigbee.switchActions,
    customToZigbee.relayMode,
    customToZigbee.relayIndex,
    customToZigbee.boundMode,
    customToZigbee.longPressDuration,
    customToZigbee.levelMoveRate,
    customToZigbee.pressAction,
    customToZigbee.networkLed,
    customToZigbee.relayIndicatorMode,
    customToZigbee.relayIndicator,
    customToZigbee.deviceConfig,
    customToZigbee.multiPressResetCount,
    customToZigbee.coverSwitchType,
    customToZigbee.coverSwitchCoverIndex,
    customToZigbee.coverSwitchInvert,
    customToZigbee.coverSwitchLocalMode,
    customToZigbee.coverSwitchBoundMode,
    customToZigbee.coverSwitchLongPressDuration,
    customToZigbee.coverSwitchPressAction,
    customToZigbee.coverMotorReversal,
  ],
  fromZigbee: [
    fz.on_off,
    fz.power_on_behavior,
    customFromZigbee.switchConfigAttrs,
    customFromZigbee.multistateInput,
    customFromZigbee.basicAttrs,
    customFromZigbee.batteryPercentage,
    customFromZigbee.relayIndicatorAttrs,
    customFromZigbee.coverSwitchConfigAttrs,
    customFromZigbee.coverAttrs,
  ],
  extend: [],
  meta: { multiEndpoint: true },
  onEvent: (event, _, device) => {
    if (event === "start") {
      registerCustomClusters(device);
    }
    if (event?.type === "start") {
      registerCustomClusters(event.data.device);
    }
  },
  configure: async (device, coordinatorEndpoint) => {
    registerCustomClusters(device);

    const mainEndpoint = device.getEndpoint(1);
    await mainEndpoint.read("genBasic", ["deviceConfig", "multiPressResetCount"]);

    const config = getDeviceConfigString(device);
    logger.debug(`Read config ${config} during configure`, NS);
    if (!config) {
      return;
    }

    const deviceConfig = parseConfig(config);
    logger.debug(
      `Parsed config as ${JSON.stringify(deviceConfig)} during configure`,
      NS
    );

    if (deviceConfig.hasNetworkLed) {
      await setupAttributes(
        mainEndpoint,
        coordinatorEndpoint,
        "genBasic",
        [{ attribute: "networkLed", min: -1, max: -1, change: -1 }],
        false
      );
    }

    if (deviceConfig.hasBatteryCluster) {
      await setupAttributes(
        mainEndpoint,
        coordinatorEndpoint,
        "genPowerCfg",
        [{ attribute: "batteryPercentageRemaining", min: 0, max: 3600, change: 2 }]
      );
      await mainEndpoint.read("genPowerCfg", ["batteryVoltage"]);
    }

    for (let i = 1; i <= deviceConfig.switchCount; i++) {
      const switchEndpoint = device.getEndpoint(i);
      await setupAttributes(
        switchEndpoint,
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
        switchEndpoint,
        coordinatorEndpoint,
        "genMultistateInput",
        [{ attribute: "presentValue", min: 0, max: 60, change: 1 }]
      );
    }

    for (let i = 1; i <= deviceConfig.relayCount; i++) {
      const relayEndpoint = device.getEndpoint(deviceConfig.switchCount + i);
      await setupAttributes(
        relayEndpoint,
        coordinatorEndpoint,
        "genOnOff",
        [{ attribute: "onOff", min: 0, max: 60, change: 1 }]
      );
      await setupAttributes(
        relayEndpoint,
        coordinatorEndpoint,
        "genOnOff",
        [{ attribute: "startUpOnOff", min: -1, max: -1, change: -1 }],
        false
      );
      if (i <= deviceConfig.relayIndicatorCount) {
        await setupAttributes(
          relayEndpoint,
          coordinatorEndpoint,
          "genOnOff",
          [
            { attribute: "relayIndicator", min: -1, max: -1, change: -1 },
            { attribute: "relayIndicatorMode", min: -1, max: -1, change: -1 },
          ],
          false
        );
      }
    }

    const coverSwitchBase = deviceConfig.switchCount + deviceConfig.relayCount;
    for (let i = 1; i <= deviceConfig.coverSwitchCount; i++) {
      const coverSwitchEndpoint = device.getEndpoint(coverSwitchBase + i);
      await setupAttributes(
        coverSwitchEndpoint,
        coordinatorEndpoint,
        "manuSpecificTuyaCoverSwitchConfig",
        [
          { attribute: "switchType", min: -1, max: -1, change: -1 },
          { attribute: "coverIndex", min: -1, max: -1, change: -1 },
          { attribute: "reversal", min: -1, max: -1, change: -1 },
          { attribute: "localMode", min: -1, max: -1, change: -1 },
          { attribute: "bindedMode", min: -1, max: -1, change: -1 },
          { attribute: "longPressDuration", min: -1, max: -1, change: -1 },
        ],
        false
      );
      await setupAttributes(
        coverSwitchEndpoint,
        coordinatorEndpoint,
        "genMultistateInput",
        [{ attribute: "presentValue", min: 0, max: 60, change: 1 }]
      );
    }

    const coverBase = coverSwitchBase + deviceConfig.coverSwitchCount;
    for (let i = 1; i <= deviceConfig.coverCount; i++) {
      const coverEndpoint = device.getEndpoint(coverBase + i);
      await setupAttributes(
        coverEndpoint,
        coordinatorEndpoint,
        "closuresWindowCovering",
        [{ attribute: "moving", min: 0, max: 60, change: 1 }]
      );
      await setupAttributes(
        coverEndpoint,
        coordinatorEndpoint,
        "closuresWindowCovering",
        [{ attribute: "motorReversal", min: -1, max: -1, change: -1 }],
        false
      );
    }
  },
  ota: true,
};

const MODELS = [
  {
    zigbeeModel: [
      "TS0004-MC",
    ],
    model: "TYWB 4ch-RF",
  },
  {
    zigbeeModel: [
      "TS0004-MC1",
    ],
    model: "TYWB 4ch-RF",
  },
  {
    zigbeeModel: [
      "TS0004-MC2",
    ],
    model: "TYWB 4ch-RF",
  },
  {
    zigbeeModel: [
      "Tuya-ZG-001",
    ],
    model: "ZG-001",
  },
  {
    zigbeeModel: [
      "TS0002-SC",
    ],
    model: "ZG-2002-RF",
  },
  {
    zigbeeModel: [
      "TS0002-SC",
    ],
    model: "ZG-2002-RF",
  },
  {
    zigbeeModel: [
      "DEV-ZTU2",
    ],
    model: "Zigbee_SoC_Board_V2_(ZTU)",
  },
  {
    zigbeeModel: [
      "WHD02-Aubess",
      "WHD02-Aubess-ED",
    ],
    model: "WHD02",
  },
  {
    zigbeeModel: [
      "TS0002-AUB",
    ],
    model: "TMZ02",
  },
  {
    zigbeeModel: [
      "TS0003-AUB",
    ],
    model: "TS0003_switch_module_2",
  },
  {
    zigbeeModel: [
      "TS0004-custom",
    ],
    model: "TS0004_switch_module",
  },
  {
    zigbeeModel: [
      "TS0001-AVB",
      "TS0001-Avatto-custom",
      "TS0001-AV-CUS",
    ],
    model: "ZWSM16-1-Zigbee",
  },
  {
    zigbeeModel: [
      "TS0002-AVB",
      "TS0002-Avatto-custom",
      "TS0002-AV-CUS",
    ],
    model: "ZWSM16-2-Zigbee",
  },
  {
    zigbeeModel: [
      "TS0003-AVB",
      "TS0003-Avatto-custom",
      "TS0003-AV-CUS",
    ],
    model: "ZWSM16-3-Zigbee",
  },
  {
    zigbeeModel: [
      "TS0004-AVB",
      "TS0004-Avatto-custom",
      "TS0004-AV-CUS",
    ],
    model: "ZWSM16-4-Zigbee",
  },
  {
    zigbeeModel: [
      "TS0003-AVB2",
    ],
    model: "ZWSM16-3-Zigbee",
  },
  {
    zigbeeModel: [
      "TS0004-AVB2",
    ],
    model: "ZWSM16-4-Zigbee",
  },
  {
    zigbeeModel: [
      "TS0001-AV-DRY",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0011-avatto",
      "TS0011-avatto-ED",
    ],
    model: "LZWSM16-1",
  },
  {
    zigbeeModel: [
      "TS0012-avatto",
      "TS0012-avatto-ED",
    ],
    model: "LZWSM16-2",
  },
  {
    zigbeeModel: [
      "TS0012-AVB1",
    ],
    model: "LZWSM16-2",
  },
  {
    zigbeeModel: [
      "TS0013-AVB",
    ],
    model: "LZWSM16-3",
  },
  {
    zigbeeModel: [
      "EKAC-T3092Z-CUSTOM",
    ],
    model: "EKAC-T3092Z",
  },
  {
    zigbeeModel: [
      "TS0012-EKF",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "TS0001-GS",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0002-GS",
    ],
    model: "TS0002",
  },
  {
    zigbeeModel: [
      "TS0002-NS",
    ],
    model: "L13Z",
  },
  {
    zigbeeModel: [
      "TS0001-FL",
      "TS0002-FL",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0003-GRA",
      "TS0003-GR",
    ],
    model: "TS0003_switch_module_2",
  },
  {
    zigbeeModel: [
      "TS0001-GRA",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0012-C",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "TS0001-GD",
    ],
    model: "TS0001_switch_module",
  },
  {
    zigbeeModel: [
      "TS0001-GIR",
    ],
    model: "JR-ZDS01",
  },
  {
    zigbeeModel: [
      "TS0002-GIR",
      "TS0002-custom",
    ],
    model: "TS0002_basic",
  },
  {
    zigbeeModel: [
      "TS130F-GIR",
    ],
    model: "TS130F_GIRIER",
  },
  {
    zigbeeModel: [
      "TS130F-GIR-DUAL",
    ],
    model: "TS130F_GIRIER_DUAL",
  },
  {
    zigbeeModel: [
      "TS0001-GIR-1",
    ],
    model: "JR-ZDS01",
  },
  {
    zigbeeModel: [
      "TS0001-HOBM",
    ],
    model: "ZG-301Z",
  },
  {
    zigbeeModel: [
      "TS0001-HOB1",
    ],
    model: "WHD02",
  },
  {
    zigbeeModel: [
      "TS0001-HOB",
    ],
    model: "ZG-301Z",
  },
  {
    zigbeeModel: [
      "TS0011-HOMMYN",
    ],
    model: "TS0011",
  },
  {
    zigbeeModel: [
      "TS0001-IHS",
    ],
    model: "_TZ3000_pgq7ormg",
  },
  {
    zigbeeModel: [
      "TS0003-IHS",
      "TS0003-3CH-cus",
    ],
    model: "_TZ3000_mhhxxjrs",
  },
  {
    zigbeeModel: [
      "TS0004-IHS",
    ],
    model: "_TZ3000_knoj8lpk",
  },
  {
    zigbeeModel: [
      "TS0001-IHA",
    ],
    model: "WHD02",
  },
  {
    zigbeeModel: [
      "TS0001-PWR",
    ],
    model: "TS0001_power",
  },
  {
    zigbeeModel: [
      "TS0002-MSB",
    ],
    model: "ZM-104B-M",
  },
  {
    zigbeeModel: [
      "TS0003-custom",
    ],
    model: "MS-104CZ",
  },
  {
    zigbeeModel: [
      "TS0011-MS",
    ],
    model: "TS0011",
  },
  {
    zigbeeModel: [
      "TS0002-MS",
    ],
    model: "ZM4LT2",
  },
  {
    zigbeeModel: [
      "TS0003-MS",
    ],
    model: "ZM4LT3",
  },
  {
    zigbeeModel: [
      "TS0004-MS",
    ],
    model: "ZM4LT4",
  },
  {
    zigbeeModel: [
      "TS0001-nous",
    ],
    model: "B1Z",
  },
  {
    zigbeeModel: [
      "MS105-ZB-CUSTOM",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "ZBMINIL2-custom",
    ],
    model: "ZBMINIL2",
  },
  {
    zigbeeModel: [
      "TS0001-TLED",
    ],
    model: "WHD02",
  },
  {
    zigbeeModel: [
      "TS0001-C",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0002-C",
    ],
    model: "TS0002_basic_2",
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
      "WHD02-custom",
    ],
    model: "WHD02",
  },
  {
    zigbeeModel: [
      "TS0001-CC",
    ],
    model: "WHD02",
  },
  {
    zigbeeModel: [
      "TS0011-custom",
    ],
    model: "TS0011_switch_module",
  },
  {
    zigbeeModel: [
      "TS0011-CUS-2",
    ],
    model: "TS0011_switch_module",
  },
  {
    zigbeeModel: [
      "TS0012-custom",
      "TS0042-CUSTOM",
      "TS0012-custom-end-device",
    ],
    model: "TS0012_switch_module",
  },
  {
    zigbeeModel: [
      "ZB08-custom",
      "ZB08-custom-ED",
    ],
    model: "ZB08",
  },
  {
    zigbeeModel: [
      "TS0001-QS-custom",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0011-S05",
    ],
    model: "TS0011_switch_module",
  },
  {
    zigbeeModel: [
      "TS0002-QS",
    ],
    model: "TS0002_limited",
  },
  {
    zigbeeModel: [
      "TS0003-Avv",
    ],
    model: "TS0003",
  },
  {
    zigbeeModel: [
      "NovatoZRM01",
    ],
    model: "QS-Zigbee-SEC01-U",
  },
  {
    zigbeeModel: [
      "NovatoZRM02",
    ],
    model: "QS-Zigbee-SEC02-U",
  },
  {
    zigbeeModel: [
      "NovatoZNR01",
    ],
    model: "TS0011",
  },
  {
    zigbeeModel: [
      "TS0012-QS",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "TS130F-NOV",
    ],
    model: "TS130F",
  },
  {
    zigbeeModel: [
      "TS0001-custom",
    ],
    model: "TS0001_switch_module",
  },
  {
    zigbeeModel: [
      "TS0002-OXT-CUS",
    ],
    model: "TS0002_basic",
  },
  {
    zigbeeModel: [
      "TS0002-custom",
    ],
    model: "TS0002_basic",
  },
  {
    zigbeeModel: [
      "TS011F-TUYA",
    ],
    model: "TS011F_plug_1",
  },
  {
    zigbeeModel: [
      "TS0001-TS",
    ],
    model: "WHD02",
  },
  {
    zigbeeModel: [
      "TS0001-ZTU",
    ],
    model: "TS0001_switch_module",
  },
  {
    zigbeeModel: [
      "TS0004-Avv",
    ],
    model: "TS0004_switch_module",
  },
  {
    zigbeeModel: [
      "TS0002-NS1",
    ],
    model: "L13Z",
  },
  {
    zigbeeModel: [
      "TS0002-ZB",
    ],
    model: "TS0002",
  },
  {
    zigbeeModel: [
      "TS011F-BS-PM",
    ],
    model: "TS011F_plug_1_2",
  },
  {
    zigbeeModel: [
      "TS011F-BS",
    ],
    model: "_TZ3000_o1jzcxou",
  },
  {
    zigbeeModel: [
      "TS011F-AB-PM",
    ],
    model: "TS011F_plug_1",
  },
  {
    zigbeeModel: [
      "TS0044-CUS",
    ],
    model: "_TZ3000_mh9px7cq",
  },
  {
    zigbeeModel: [
      "TS0044-MOES",
    ],
    model: "TS0044",
  },
  {
    zigbeeModel: [
      "TS004F-Loginovo",
    ],
    model: "ZG-101ZL",
  },
  {
    zigbeeModel: [
      "TS004F-TUYA",
    ],
    model: "TS004F",
  },
  {
    zigbeeModel: [
      "TS0001-AVT",
    ],
    model: "RoomsAI_37022454",
  },
  {
    zigbeeModel: [
      "TS0002-AVT",
    ],
    model: "37022463-2",
  },
  {
    zigbeeModel: [
      "TS0003-AVT",
      "Avatto-3-touch",
    ],
    model: "370224742",
  },
  {
    zigbeeModel: [
      "TS0004-AVT",
    ],
    model: "TS0004",
  },
  {
    zigbeeModel: [
      "TS0001-BSDB",
      "TS0001-BS-T",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0002-BSDB",
      "TS0002-BS-1",
    ],
    model: "TS0002",
  },
  {
    zigbeeModel: [
      "TS0003-BSDB",
      "TS0003-BSEED",
    ],
    model: "TS0003",
  },
  {
    zigbeeModel: [
      "BSLR1",
    ],
    model: "TS0011",
  },
  {
    zigbeeModel: [
      "BSLR2",
      "Bseed-2-gang-2",
      "Bseed-2-gang-2-ED",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "BSLR3",
      "TS0013-2-BS",
    ],
    model: "TS0013",
  },
  {
    zigbeeModel: [
      "TS0001-BSMN",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0003-BSMN",
      "TS0003-BS",
    ],
    model: "TS0003",
  },
  {
    zigbeeModel: [
      "TS0004-BSMN",
      "TS0004-BS",
    ],
    model: "TS0004",
  },
  {
    zigbeeModel: [
      "TS0001-CUS-T",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0002-CUS-T",
    ],
    model: "TS0002",
  },
  {
    zigbeeModel: [
      "TS0011-BS-T",
    ],
    model: "TS0011",
  },
  {
    zigbeeModel: [
      "Bseed-2-gang",
      "Bseed-2-gang-ED",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "Bseed-2-gang-3",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "TS0013-BS",
    ],
    model: "TS0013",
  },
  {
    zigbeeModel: [
      "TS0726-1-BS",
    ],
    model: "EC-GL86ZPCS11",
  },
  {
    zigbeeModel: [
      "TS0726-2-BS",
    ],
    model: "EC-GL86ZPCS21",
  },
  {
    zigbeeModel: [
      "TS0726-3-BS",
    ],
    model: "EC-GL86ZPCS31",
  },
  {
    zigbeeModel: [
      "BS4",
      "TS0726-4-BS",
    ],
    model: "EC-GL86ZPCS41",
  },
  {
    zigbeeModel: [
      "TS0726-1-BSL",
    ],
    model: "EC-SL-FK86ZPCS11",
  },
  {
    zigbeeModel: [
      "TS0726-2-BSL",
    ],
    model: "EC-SL-FK86ZPCS21",
  },
  {
    zigbeeModel: [
      "TS0001-HBS",
    ],
    model: "TS0601_switch_1_gang",
  },
  {
    zigbeeModel: [
      "TS0001-HMT",
    ],
    model: "Homeetec_37022454",
  },
  {
    zigbeeModel: [
      "TS0002-HMT",
    ],
    model: "37022463-1",
  },
  {
    zigbeeModel: [
      "TS0003-HMT",
    ],
    model: "37022474_1",
  },
  {
    zigbeeModel: [
      "TS0001-IHS-T",
    ],
    model: "_TZ3000_qq9ahj6z",
  },
  {
    zigbeeModel: [
      "TS0002-IHS-T",
    ],
    model: "_TZ3000_zxrfobzw",
  },
  {
    zigbeeModel: [
      "TS0003-IHS-T",
    ],
    model: "TW-03",
  },
  {
    zigbeeModel: [
      "LerLink-2-gang",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "LerLink-3-gang",
    ],
    model: "TS0013",
  },
  {
    zigbeeModel: [
      "TS0011-MH",
    ],
    model: "TS0011",
  },
  {
    zigbeeModel: [
      "TS0012-MH",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "TS0013-MH",
    ],
    model: "TS0013",
  },
  {
    zigbeeModel: [
      "TS0011-MHB",
    ],
    model: "TS0011",
  },
  {
    zigbeeModel: [
      "TS0012-MHB",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "TS0013-MHB",
    ],
    model: "TS0013",
  },
  {
    zigbeeModel: [
      "Moes-1-gang",
      "Moes-1-gang-ED",
    ],
    model: "ZS-EUB_1gang",
  },
  {
    zigbeeModel: [
      "Moes-2-gang",
      "Moes-2-gang-ED",
    ],
    model: "ZS-EUB_2gang",
  },
  {
    zigbeeModel: [
      "Moes-3-gang",
      "Moes-3-gang-ED",
    ],
    model: "TS0013",
  },
  {
    zigbeeModel: [
      "MS4",
    ],
    model: "TS0014",
  },
  {
    zigbeeModel: [
      "MS33",
    ],
    model: "SR-ZS",
  },
  {
    zigbeeModel: [
      "ZTS-3W-CUSTOM",
    ],
    model: "WS-US-ZB",
  },
  {
    zigbeeModel: [
      "TS0001-PST",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0002-PST",
    ],
    model: "TS0002",
  },
  {
    zigbeeModel: [
      "TS0001-PS",
      "T441",
    ],
    model: "T441",
  },
  {
    zigbeeModel: [
      "TS0002-PS",
      "T442",
    ],
    model: "T442",
  },
  {
    zigbeeModel: [
      "TS0003-PS",
    ],
    model: "ZM-L03E-Z",
  },
  {
    zigbeeModel: [
      "TS0001-CUS",
    ],
    model: "TS0001",
  },
  {
    zigbeeModel: [
      "TS0004-CUS",
    ],
    model: "TS0004",
  },
  {
    zigbeeModel: [
      "Zemi-2-gang",
      "Zemi-2-gang-ED",
    ],
    model: "TS0012",
  },
  {
    zigbeeModel: [
      "TS0011-ZS",
    ],
    model: "TS0011",
  },
  {
    zigbeeModel: [
      "TS0012-ZS",
    ],
    model: "TS0012",
  },
];

const definitions = MODELS.map((model) => ({ ...model, ...baseDefinition }));

export default definitions;
