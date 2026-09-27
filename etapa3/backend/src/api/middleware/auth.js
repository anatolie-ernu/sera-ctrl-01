'use strict';
const jwt = require('jsonwebtoken');
function auth(req, res, next) {
    const h = req.headers.authorization || '';
    const t = h.startsWith('Bearer ') ? h.slice(7) : null;
    if (!t) return res.status(401).json({error:'Token lipsa'});
    try { req.user = jwt.verify(t, process.env.JWT_SECRET); next(); }
    catch { res.status(401).json({error:'Token invalid sau expirat'}); }
}
function adminOnly(req, res, next) {
    if (req.user?.role !== 'admin') return res.status(403).json({error:'Necesita admin'});
    next();
}
module.exports = { auth, adminOnly };
