'use strict';
const router = require('express').Router();
const db     = require('../../db/pool');
const {auth} = require('../middleware/auth');
router.get('/', auth, async (req,res) => {
    const devs = await db.query('SELECT * FROM devices ORDER BY device_id');
    const now  = Date.now();
    res.json({data: devs.map(d=>({...d,online:d.last_seen&&(now-new Date(d.last_seen).getTime())<120000}))});
});
module.exports = router;
