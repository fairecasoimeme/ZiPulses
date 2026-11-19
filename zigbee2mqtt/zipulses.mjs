// External converter for the LiXee ZiPulses running a firmware with the configurable power mode.
// Copy of the LiXee definition from zigbee-herdsman-converters 26.125.0 (src/devices/lixee.ts),
// plus a "power_mode" setting (battery / mains) written to the standard BatterySize attribute
// of the Power Configuration cluster (0 = no battery = external power).
// Install: copy to <zigbee2mqtt data>/external_converters/ and restart zigbee2mqtt.
// The device sleeps: press its link button just before changing a setting.

import * as fz from "zigbee-herdsman-converters/converters/fromZigbee";
import * as exposes from "zigbee-herdsman-converters/lib/exposes";
import * as m from "zigbee-herdsman-converters/lib/modernExtend";
import * as reporting from "zigbee-herdsman-converters/lib/reporting";

const e = exposes.presets;
const ea = exposes.access;

const unitsZiPulses = [
    "kWh",
    "m3",
    "ft3",
    "ccf",
    "US gl",
    "IMP gl",
    "BTUs",
    "L (litre)",
    "kPA (jauge)",
    "kPA (absolu)",
    "kPA (absolu)",
    "sans unité",
    "MJ",
    "kVar",
];

// BatterySize values: 0 = no battery (external power), 2 = other (CR2477)
const BATTERY_SIZE_NO_BATTERY = 0;
const BATTERY_SIZE_OTHER = 2;

const tzSeMetering = {
    key: ["divisor", "multiplier", "unitOfMeasure"],
    convertSet: async (entity, key, value, meta) => {
        if (key === "unitOfMeasure") {
            const val = unitsZiPulses.indexOf(value);
            const payload = {768: {value: val, type: 48}};
            await entity.write("seMetering", payload);
            await entity.read("seMetering", [key]);
            return {state: {unitOfMeasure: value}};
        }
        await entity.write("seMetering", {[key]: value});
        return {state: {[key]: value}};
    },
};

const tzPowerMode = {
    key: ["power_mode"],
    convertSet: async (entity, key, value, meta) => {
        const batterySize = value === "mains" ? BATTERY_SIZE_NO_BATTERY : BATTERY_SIZE_OTHER;
        await entity.write("genPowerCfg", {batterySize});
        await entity.read("genPowerCfg", ["batterySize"]);
        return {state: {power_mode: value}};
    },
    convertGet: async (entity, key, meta) => {
        await entity.read("genPowerCfg", ["batterySize"]);
    },
};

const fzZiPulses = {
    cluster: "seMetering",
    type: ["attributeReport", "readResponse"],
    convert: (model, msg, publish, options, meta) => {
        const payload = {};
        if (msg.data.multiplier !== undefined) {
            payload.multiplier = msg.data.multiplier;
        }
        if (msg.data.divisor !== undefined) {
            payload.divisor = msg.data.divisor;
        }
        if (msg.data.unitOfMeasure !== undefined) {
            payload.unitOfMeasure = unitsZiPulses[msg.data.unitOfMeasure];
        }
        return payload;
    },
};

const fzPowerMode = {
    cluster: "genPowerCfg",
    type: ["attributeReport", "readResponse"],
    convert: (model, msg, publish, options, meta) => {
        if (msg.data.batterySize !== undefined) {
            return {power_mode: msg.data.batterySize === BATTERY_SIZE_NO_BATTERY ? "mains" : "battery"};
        }
    },
};

export default {
    zigbeeModel: ["ZiPulses"],
    model: "ZiPulses",
    vendor: "LiXee",
    description: "Lixee ZiPulses (modified firmware, configurable power mode)",
    fromZigbee: [fz.battery, fz.temperature, fz.metering, fzZiPulses, fzPowerMode],
    toZigbee: [tzSeMetering, tzPowerMode],
    extend: [
        m.deviceAddCustomCluster("seMetering", {
            name: "seMetering",
            ID: 0x0702,
            attributes: {
                unitOfMeasure: {name: "unitOfMeasure", ID: 0x0300, type: 0x30, required: true, max: 0xff, default: 0x00, write: true},
                multiplier: {name: "multiplier", ID: 0x0301, type: 0x22, max: 0xffffff, write: true},
                divisor: {name: "divisor", ID: 0x0302, type: 0x22, max: 0xffffff, write: true},
            },
            commands: {},
            commandsResponse: {},
        }),
    ],
    exposes: [
        e.battery(),
        e.battery_voltage(),
        e.temperature(),
        e.numeric("multiplier", ea.STATE_SET).withValueMin(1).withValueMax(1000).withDescription("It is necessary to press the link button to update"),
        e.numeric("divisor", ea.STATE_SET).withValueMin(1).withValueMax(1000).withDescription("It is necessary to press the link button to update"),
        e.enum("unitOfMeasure", ea.STATE_SET, unitsZiPulses).withDescription("It is necessary to press the link button to update"),
        e.numeric("energy", ea.STATE),
        e
            .enum("power_mode", ea.ALL, ["battery", "mains"])
            .withDescription("battery: wake every 60 s; mains: wake every 5 s while water flows. Press the link button just before changing it"),
    ],
    configure: async (device, coordinatorEndpoint) => {
        const endpoint = device.getEndpoint(1);
        const binds = ["genPowerCfg", "seMetering", "msTemperatureMeasurement"];
        await reporting.bind(endpoint, coordinatorEndpoint, binds);
        await endpoint.read("seMetering", ["divisor", "unitOfMeasure", "multiplier"]);
        await endpoint.read("genPowerCfg", ["batterySize"]);
    },
};
