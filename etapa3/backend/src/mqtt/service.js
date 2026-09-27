'use strict';
const mqtt       = require('mqtt');
const logger     = require('../utils/logger');
const db         = require('../db/pool');

let client, _connected = false;
const DEVICE_ID = process.env.DEVICE_ID || 'SERA_001';

async function connect() {
    return new Promise((resolve, reject) => {
        client = mqtt.connect(`mqtt://${process.env.MQTT_HOST||'localhost'}:${process.env.MQTT_PORT||1883}`, {
            username: process.env.MQTT_USER, password: process.env.MQTT_PASS,
            clientId: `sera_backend_${Date.now()}`, keepalive: 60,
            reconnectPeriod: 5000, connectTimeout: 10000,
        });
        client.on('connect', () => {
            _connected = true; logger.info('MQTT conectat');
            client.subscribe('greenhouse/+/sensors');
            client.subscribe('greenhouse/+/heartbeat');
            client.subscribe('greenhouse/+/alerts');
            resolve();
        });
        client.on('message', handleMessage);
        client.on('error', e => logger.error('MQTT:', e.message));
        client.on('offline', () => { _connected = false; logger.warn('MQTT offline'); });
        setTimeout(() => reject(new Error('MQTT timeout')), 15000);
    });
}

async function handleMessage(topic, payload) {
    let data;
    try { data = JSON.parse(payload.toString()); } catch { return; }
    const parts = topic.split('/');
    const deviceId = parts[1];
    const type = parts[2];
    if (type === 'sensors')   await saveSensors(deviceId, data);
    if (type === 'heartbeat') await saveHeartbeat(deviceId, data);
    if (type === 'alerts')    await saveAlert(deviceId, data);
}

async function saveSensors(deviceId, d) {
    try {
        await db.query(`
            INSERT INTO sensor_readings
              (time,device_id,temperature,humidity,soil_raw,soil_pct,dht_valid,soil_valid,wifi_rssi)
            VALUES (NOW(),$1,$2,$3,$4,$5,$6,$7,$8)`,
            [deviceId, d.temp??null, d.hum??null, d.soil_raw??null,
             d.soil_pct??null, d.dht_ok??true, d.soil_ok??true,
             d.state?.wifi_rssi??null]);
        // Evalueaza alerte
        const { evaluateAlerts } = require('../alerts/engine');
        await evaluateAlerts(deviceId, {temperature:d.temp, humidity:d.hum, soil_pct:d.soil_pct});
        logger.debug(`Senzori salvati: ${deviceId} T=${d.temp} H=${d.hum}`);
    } catch(e) { logger.error('saveSensors:', e.message); }
}

async function saveHeartbeat(deviceId, d) {
    try {
        await db.query(`
            INSERT INTO devices (device_id,online,last_seen) VALUES ($1,true,NOW())
            ON CONFLICT (device_id) DO UPDATE SET online=true,last_seen=NOW()`,
            [deviceId]);
    } catch(e) { logger.error('saveHeartbeat:', e.message); }
}

async function saveAlert(deviceId, d) {
    try {
        await db.query(
            `INSERT INTO alert_history (time,device_id,message,severity) VALUES (NOW(),$1,$2,'critical')`,
            [deviceId, d.message||JSON.stringify(d)]);
    } catch(e) { logger.error('saveAlert:', e.message); }
}

function sendCommand(deviceId, type, payload) {
    if (!_connected) { logger.warn('MQTT neconectat'); return false; }
    const topic = `greenhouse/${deviceId}/cmd/${type}`;
    client.publish(topic, typeof payload==='string'?payload:JSON.stringify(payload), {qos:1});
    logger.info(`Comanda: ${topic} → ${JSON.stringify(payload)}`);
    return true;
}

async function disconnect() { if(client){client.end(true);_connected=false;} }
function isConnected() { return _connected; }
module.exports = { connect, sendCommand, disconnect, isConnected };
