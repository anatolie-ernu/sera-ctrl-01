/**
 * fleet.js — Managementul actualizărilor OTA pentru flotă
 * ─────────────────────────────────────────────────────────────────────────────
 * Doar pentru administratorul PLATFORMEI (producătorul), nu pentru clienți.
 *
 * Rollout eșalonat: deployRelease() pune dispozitivele în coadă și un worker
 * trimite comenzi cmd/ota în loturi de max 10%/oră — evită avalanșa pe
 * serverul de update și limitează raza de impact a unei versiuni proaste.
 */
'use strict';
const router = require('express').Router();
const db     = require('../../db/pool');
const mqtt   = require('../../mqtt/service');
const { auth, adminOnly } = require('../middleware/auth');

// Publică o versiune nouă (CI o apelează după semnare + upload .bin)
router.post('/releases', auth, adminOnly, async (req, res) => {
    const { version, url, sha256, channel = 'beta', notes = '' } = req.body;
    if (!version || !url || !sha256 || sha256.length !== 64)
        return res.status(400).json({ error: 'version, url, sha256(64 hex) obligatorii' });
    const r = await db.query(`
        INSERT INTO firmware_releases (version,url,sha256,channel,notes)
        VALUES ($1,$2,$3,$4,$5) RETURNING id`,
        [version, url, sha256.toLowerCase(), channel, notes]);
    res.status(201).json({ id: r[0].id, version, channel });
});

// Promovează beta → stable după perioada de observație
router.put('/releases/:version/promote', auth, adminOnly, async (req, res) => {
    await db.query(
        `UPDATE firmware_releases SET channel='stable' WHERE version=$1`,
        [req.params.version]);
    res.json({ message: `${req.params.version} promovat pe stable` });
});

// Pornește rollout-ul: pune în coadă toate dispozitivele online sub versiune
router.post('/deploy/:version', auth, adminOnly, async (req, res) => {
    const rel = (await db.query(
        `SELECT * FROM firmware_releases WHERE version=$1`, [req.params.version]))[0];
    if (!rel) return res.status(404).json({ error: 'Versiune inexistentă' });

    const queued = await db.query(`
        INSERT INTO device_firmware_status (device_id, target_ver, ota_state)
        SELECT d.device_id, $1, 'queued'
        FROM devices d
        WHERE d.online = true
        ON CONFLICT (device_id) DO UPDATE
            SET target_ver=$1, ota_state='queued', updated_at=NOW()
        RETURNING device_id`, [rel.version]);
    res.json({ message: `${queued.length} dispozitive în coadă pentru v${rel.version}` });
});

// Worker (apelat de un cron la fiecare oră): trimite următorul lot de 10%
async function processOtaQueue() {
    const rel = (await db.query(
        `SELECT * FROM firmware_releases ORDER BY released_at DESC LIMIT 1`))[0];
    if (!rel) return;
    const fleet = (await db.query(`SELECT COUNT(*) n FROM device_firmware_status`))[0];
    const batch = Math.max(1, Math.ceil(fleet.n * 0.10));        // 10% / oră

    const targets = await db.query(`
        SELECT device_id FROM device_firmware_status
        WHERE ota_state='queued' LIMIT $1`, [batch]);
    for (const t of targets) {
        mqtt.sendCommand(t.device_id, 'ota',
            { url: rel.url, version: rel.version, sha256: rel.sha256 });
        await db.query(`UPDATE device_firmware_status
            SET ota_state='sent', updated_at=NOW() WHERE device_id=$1`,
            [t.device_id]);
    }
}

// Tabloul de bord al flotei: câte dispozitive pe fiecare versiune/stare
router.get('/status', auth, adminOnly, async (req, res) => {
    const rows = await db.query(`
        SELECT current_ver, ota_state, COUNT(*) AS devices
        FROM device_firmware_status
        GROUP BY current_ver, ota_state ORDER BY current_ver`);
    res.json({ data: rows });
});

module.exports = { router, processOtaQueue };
