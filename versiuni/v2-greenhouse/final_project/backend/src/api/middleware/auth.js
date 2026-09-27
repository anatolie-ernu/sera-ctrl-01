/**
 * api/middleware/auth.js  —  Greenhouse IoT System | Backend
 * ─────────────────────────────────────────────────────────────────────────────
 * Express middleware for JWT authentication and role-based access control.
 *
 * Token format
 * ─────────────────────────────────────────────────────────────────────────────
 * The client includes the access token in every request:
 *   Authorization: Bearer <access_token>
 *
 * The access token is a JWT signed with JWT_SECRET containing:
 *   { id, username, role, iat, exp }
 *
 * auth middleware:
 *   • Extracts the token from the Authorization header
 *   • Verifies the signature and expiry with jwt.verify()
 *   • Attaches the decoded payload to req.user
 *   • Returns 401 if the token is missing, invalid, or expired
 *
 * adminOnly middleware:
 *   • Must be used after auth (depends on req.user being set)
 *   • Returns 403 if req.user.role !== 'admin'
 *   • Used on endpoints that modify data (create/update/delete schedules, rules)
 */
'use strict';

const jwt = require('jsonwebtoken');

/**
 * Require a valid Bearer JWT access token.
 * Attaches the decoded payload to req.user on success.
 */
function auth(req, res, next) {
    const authHeader = req.headers.authorization || '';
    const token      = authHeader.startsWith('Bearer ') ? authHeader.slice(7) : null;

    if (!token) {
        return res.status(401).json({ error: 'Authentication required — no token provided' });
    }

    try {
        // jwt.verify() throws if:
        //   • The signature is invalid (token was tampered with)
        //   • The token has expired (exp < now)
        //   • The token format is malformed
        req.user = jwt.verify(token, process.env.JWT_SECRET);
        next();
    } catch (err) {
        // Return the same 401 for all failure reasons to avoid leaking info
        res.status(401).json({ error: 'Invalid or expired token — please log in again' });
    }
}

/**
 * Require admin role.  Must be called after auth().
 * Viewer accounts can read all data but cannot modify schedules or rules.
 */
function adminOnly(req, res, next) {
    if (!req.user || req.user.role !== 'admin') {
        return res.status(403).json({ error: 'Admin role required for this operation' });
    }
    next();
}

module.exports = { auth, adminOnly };
