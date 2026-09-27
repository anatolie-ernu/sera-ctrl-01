/**
 * metrics.js — Expune metrici Prometheus la GET /metrics
 * ─────────────────────────────────────────────────────────────────────────────
 * Montare in app.js:  app.use('/metrics', require('../monitoring/metrics'));
 * Prometheus scrape-uieste acest endpoint la 15s. Grafana citeste din Prometheus.
 *
 * Metrici expuse:
 *   sera_devices_online      — gauge: dispozitive online acum
 *   sera_devices_total       — gauge: total dispozitive inrolate
 *   sera_sensor_temp         — gauge per device: ultima temperatura
 *   sera_mqtt_messages_total — counter: mesaje MQTT procesate
 *   sera_alerts_total        — counter: alerte declansate (pe severitate)
 *   sera_ota_state           — gauge: dispozitive pe fiecare stare OTA
 */
'use strict';
const router = require('express').Router();
const db     = require('../src/db/pool');

// Contoare in memorie (incrementate din mqtt/service.js si alerts/engine.js)
const counters = { mqtt_messages: 0, alerts: { info: 0, warning: 0, critical: 0 } };
function incMqtt()           { counters.mqtt_messages++; }
function incAlert(severity)  { counters.alerts[severity] = (counters.alerts[severity] || 0) + 1; }

// Formateaza o linie in format Prometheus exposition
const line = (name, val, labels = '') =>
  `${name}${labels ? `{${labels}}` : ''} ${val}\n`;

router.get('/', async (req, res) => {
  let out = '';
  try {
    // Dispozitive online/total
    const dev = await db.query(`
      SELECT COUNT(*) total,
             COUNT(*) FILTER (WHERE online) online
      FROM devices`);
    out += '# HELP sera_devices_total Total devices enrolled\n# TYPE sera_devices_total gauge\n';
    out += line('sera_devices_total', dev[0].total);
    out += '# HELP sera_devices_online Devices currently online\n# TYPE sera_devices_online gauge\n';
    out += line('sera_devices_online', dev[0].online);

    // Ultima temperatura per dispozitiv
    const temps = await db.query(`
      SELECT DISTINCT ON (device_id) device_id, temperature
      FROM sensor_readings ORDER BY device_id, time DESC`);
    out += '# HELP sera_sensor_temp Last temperature per device\n# TYPE sera_sensor_temp gauge\n';
    for (const t of temps)
      if (t.temperature != null)
        out += line('sera_sensor_temp', t.temperature, `device="${t.device_id}"`);

    // Stari OTA ale flotei
    const ota = await db.query(`
      SELECT ota_state, COUNT(*) n FROM device_firmware_status GROUP BY ota_state`);
    out += '# HELP sera_ota_state Devices per OTA state\n# TYPE sera_ota_state gauge\n';
    for (const o of ota) out += line('sera_ota_state', o.n, `state="${o.ota_state}"`);

    // Contoare cumulative
    out += '# HELP sera_mqtt_messages_total MQTT messages processed\n# TYPE sera_mqtt_messages_total counter\n';
    out += line('sera_mqtt_messages_total', counters.mqtt_messages);
    out += '# HELP sera_alerts_total Alerts fired\n# TYPE sera_alerts_total counter\n';
    for (const [sev, n] of Object.entries(counters.alerts))
      out += line('sera_alerts_total', n, `severity="${sev}"`);

    res.set('Content-Type', 'text/plain; version=0.0.4').send(out);
  } catch (e) {
    res.status(500).send(`# error: ${e.message}\n`);
  }
});

module.exports = router;
module.exports.incMqtt = incMqtt;
module.exports.incAlert = incAlert;
