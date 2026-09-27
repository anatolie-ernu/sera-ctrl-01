/**
 * mqtt/service.js  —  Greenhouse IoT System | Backend
 * ─────────────────────────────────────────────────────────────────────────────
 * MQTT client that bridges the ESP32 devices and the backend application.
 *
 * Responsibilities
 * ─────────────────────────────────────────────────────────────────────────────
 * INBOUND (device → server):
 *   greenhouse/+/sensors    — parse JSON, write to TimescaleDB, trigger alerts
 *   greenhouse/+/heartbeat  — update device online status in DB
 *   greenhouse/+/alerts     — store critical device-side alerts
 *
 * OUTBOUND (server → device):
 *   sendCommand(deviceId, type, payload) — publish to greenhouse/<id>/cmd/<type>
 *
 * The '+' wildcard in topic subscriptions allows this single backend instance
 * to manage multiple ESP32 devices (SERA_001, SERA_002, etc.) without
 * configuration changes.
 *
 * Connection management
 * ─────────────────────────────────────────────────────────────────────────────
 * The mqtt.js library handles automatic reconnection internally when
 * reconnectPeriod > 0.  The 'offline' event fires when the connection drops;
 * the 'connect' event fires when it is restored (including after reconnect),
 * at which point subscriptions are re-established.
 */
'use strict';

const mqtt   = require('mqtt');
const logger = require('../utils/logger');
const db     = require('../db/pool');

let client;
let _connected = false;

// ─────────────────────────────────────────────────────────────────────────────
/** Connect to the Mosquitto broker and subscribe to device topics. */
async function connect() {
    return new Promise((resolve, reject) => {
        const url = `mqtt://${process.env.MQTT_HOST || 'localhost'}:${process.env.MQTT_PORT || 1883}`;

        client = mqtt.connect(url, {
            username:        process.env.MQTT_USER,
            password:        process.env.MQTT_PASS,
            clientId:        `sera_backend_${Date.now()}`,  // Unique client ID
            keepalive:       60,         // Send ping every 60 s to keep connection alive
            reconnectPeriod: 5000,       // Automatically retry every 5 s on disconnect
            connectTimeout:  10_000,     // Fail if no CONNACK within 10 s
        });

        client.on('connect', () => {
            _connected = true;
            logger.info('MQTT connected to broker');

            // Re-subscribe on every connect (needed after reconnect)
            client.subscribe('greenhouse/+/sensors',   { qos: 1 });
            client.subscribe('greenhouse/+/heartbeat', { qos: 1 });
            client.subscribe('greenhouse/+/alerts',    { qos: 1 });

            resolve();  // Resolve the promise on first connection
        });

        client.on('message', handleMessage);
        client.on('error',   (e) => logger.error('MQTT error:', e.message));
        client.on('offline', ()  => { _connected = false; logger.warn('MQTT broker offline'); });
        client.on('reconnect', () => logger.info('MQTT reconnecting...'));

        setTimeout(() => reject(new Error('MQTT connection timeout')), 15_000);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Route incoming MQTT messages to the appropriate handler.
 * Extracts the device ID from position [1] of the topic parts:
 *   greenhouse / SERA_001 / sensors  →  parts[1] = "SERA_001"
 */
async function handleMessage(topic, payload) {
    let data;
    try {
        data = JSON.parse(payload.toString());
    } catch {
        logger.warn(`Non-JSON payload on topic ${topic}: ${payload.toString().substring(0, 80)}`);
        return;
    }

    const parts    = topic.split('/');
    const deviceId = parts[1];
    const msgType  = parts[2];

    switch (msgType) {
        case 'sensors':   await saveSensorReading(deviceId, data);  break;
        case 'heartbeat': await updateDeviceStatus(deviceId, data); break;
        case 'alerts':    await storeDeviceAlert(deviceId, data);   break;
        default: logger.debug(`Unhandled topic: ${topic}`);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Persist one sensor reading to the TimescaleDB hypertable.
 * The hypertable auto-partitions by day, so INSERT performance stays constant
 * regardless of how many historical rows exist.
 *
 * After saving, the alert engine is called synchronously so that threshold
 * violations trigger notifications within the same event loop tick as the
 * data arrives.
 */
async function saveSensorReading(deviceId, d) {
    try {
        await db.query(`
            INSERT INTO sensor_readings
              (time, device_id, temperature, humidity,
               soil_raw, soil_pct, dht_valid, soil_valid, wifi_rssi)
            VALUES (NOW(), $1, $2, $3, $4, $5, $6, $7, $8)`,
            [
                deviceId,
                d.temp      ?? null,   // $2  — null if device reported invalid reading
                d.hum       ?? null,   // $3
                d.soil_raw  ?? null,   // $4
                d.soil_pct  ?? null,   // $5
                d.dht_ok    ?? true,   // $6
                d.soil_ok   ?? true,   // $7
                d.state?.wifi_rssi ?? null,  // $8
            ]
        );

        // Evaluate alert rules for this reading
        const { evaluateAlerts } = require('../alerts/engine');
        await evaluateAlerts(deviceId, {
            temperature: d.temp,
            humidity:    d.hum,
            soil_pct:    d.soil_pct,
        });

        logger.debug(`Saved sensor reading: ${deviceId} T=${d.temp} H=${d.hum} Soil=${d.soil_pct}%`);
    } catch (err) {
        logger.error('saveSensorReading error:', err.message);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/** Update (or insert) the device's online status and last-seen timestamp. */
async function updateDeviceStatus(deviceId, data) {
    try {
        await db.query(`
            INSERT INTO devices (device_id, online, last_seen)
            VALUES ($1, true, NOW())
            ON CONFLICT (device_id)
            DO UPDATE SET online = true, last_seen = NOW()`,
            [deviceId]
        );
    } catch (err) {
        logger.error('updateDeviceStatus error:', err.message);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/** Store a critical alert published directly by the ESP32 firmware. */
async function storeDeviceAlert(deviceId, data) {
    logger.warn(`Device alert from ${deviceId}: ${data.type} — ${data.message}`);
    try {
        await db.query(`
            INSERT INTO alert_history (time, device_id, message, severity)
            VALUES (NOW(), $1, $2, 'critical')`,
            [deviceId, data.message || JSON.stringify(data)]
        );
    } catch (err) {
        logger.error('storeDeviceAlert error:', err.message);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Publish a command to a device.
 *
 * @param {string}        deviceId  Target device (e.g. "SERA_001")
 * @param {string}        type      Command type: "windows", "pump", "fan", "auto"
 * @param {Object|string} payload   JSON object or plain string
 * @returns {boolean}     true if the message was queued, false if MQTT is offline
 */
function sendCommand(deviceId, type, payload) {
    if (!_connected) {
        logger.warn(`sendCommand: MQTT not connected — command "${type}" dropped`);
        return false;
    }

    const topic = `greenhouse/${deviceId}/cmd/${type}`;
    const msg   = typeof payload === 'string' ? payload : JSON.stringify(payload);

    // QoS 1: broker stores the message until the device acknowledges receipt
    client.publish(topic, msg, { qos: 1 });
    logger.info(`MQTT command → ${topic}: ${msg}`);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
async function disconnect() {
    if (client) {
        client.end(true);   // force=true: close without waiting for in-flight messages
        _connected = false;
        logger.info('MQTT disconnected');
    }
}

function isConnected() { return _connected; }

module.exports = { connect, sendCommand, disconnect, isConnected };
