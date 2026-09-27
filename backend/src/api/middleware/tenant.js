/**
 * tenant.js — Multi-tenancy middleware
 * ─────────────────────────────────────────────────────────────────────────────
 * Runs AFTER auth: takes org_id from the JWT and injects it into the
 * PostgreSQL session so Row-Level Security policies apply automatically.
 *
 * Every tenant-scoped query MUST run inside withTenant(), which wraps the
 * request in a transaction with `SET LOCAL app.org_id` — LOCAL means the
 * setting dies with the transaction, so pooled connections never leak a
 * tenant context to the next request.
 */
'use strict';
const db = require('../../db/pool');

/** Express middleware: copy org_id from JWT onto req for downstream use. */
function tenant(req, res, next) {
    if (!req.user?.org_id) {
        return res.status(403).json({ error: 'Token fără organizație — re-autentificați-vă' });
    }
    req.orgId = req.user.org_id;
    next();
}

/**
 * Run `fn(client)` inside a transaction with the tenant context applied.
 * RLS policies (USING org_id = current_org_id()) filter every SELECT/UPDATE.
 *
 * @example
 * const devices = await withTenant(req.orgId, (c) =>
 *     c.query('SELECT * FROM devices').then(r => r.rows));
 */
async function withTenant(orgId, fn) {
    return db.transaction(async (client) => {
        // Parameterised to prevent injection even though orgId comes from JWT
        await client.query("SELECT set_config('app.org_id', $1, true)", [String(orgId)]);
        return fn(client);
    });
}

module.exports = { tenant, withTenant };
