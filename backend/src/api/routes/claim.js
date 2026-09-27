/**
 * claim.js — Înrolarea dispozitivelor (QR de pe etichetă → contul clientului)
 * ─────────────────────────────────────────────────────────────────────────────
 * Flow:
 *   1. Jigul de producție a înregistrat (device_id, claim_code) la fabricare;
 *      sincronizarea producție→server populează tabela devices cu org_id NULL.
 *   2. Clientul scanează QR-ul: https://app.sera.md/claim?d=SERA-XXXX&c=CODE
 *   3. Aplicația apelează POST /api/claim cu tokenul clientului.
 *   4. Dispozitivul primește org_id-ul clientului → apare în contul lui.
 *
 * Securitate: claim-ul reușește o singură dată (claimed_at IS NULL) și doar
 * cu codul corect — un atacator nu poate „fura" dispozitive ghicind serii.
 */
'use strict';
const router = require('express').Router();
const db     = require('../../db/pool');
const { auth }   = require('../middleware/auth');
const { tenant } = require('../middleware/tenant');

router.post('/', auth, tenant, async (req, res) => {
    const { device_id, claim_code } = req.body;
    if (!device_id || !claim_code)
        return res.status(400).json({ error: 'device_id și claim_code obligatorii' });

    try {
        // UPDATE atomic: reușește doar dacă dispozitivul există, codul e corect
        // și NU a fost deja revendicat. Notă: rulăm fără context RLS pentru
        // devices nerevendicate (org_id NULL nu trece de politică) — folosim
        // o interogare directă cu toate condițiile în WHERE.
        const rows = await db.query(`
            UPDATE devices
            SET org_id = $1, claimed_at = NOW(), name = COALESCE($4, name)
            WHERE device_id = $2
              AND claim_code = $3
              AND claimed_at IS NULL
            RETURNING device_id, name`,
            [req.orgId, device_id, claim_code.toUpperCase(), req.body.name || null]
        );

        if (!rows.length) {
            // Răspuns identic pentru toate cauzele — nu divulgăm care serie există
            return res.status(404).json({
                error: 'Cod invalid sau dispozitiv deja înrolat. Verificați eticheta.' });
        }
        res.json({ message: 'Dispozitiv adăugat în contul dvs.', device: rows[0] });
    } catch (e) {
        res.status(500).json({ error: e.message });
    }
});

module.exports = router;
