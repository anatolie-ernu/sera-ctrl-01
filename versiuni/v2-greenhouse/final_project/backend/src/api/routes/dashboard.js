'use strict';
const router = require('express').Router();
const db     = require('../../db/pool');
const {auth} = require('../middleware/auth');

router.get('/', auth, async (req, res) => {
    const device = req.query.device || 'SERA_001';
    try {
        const [latest, stats, events, alerts, dev] = await Promise.all([
            db.queryOne('SELECT * FROM sensor_readings WHERE device_id=$1 ORDER BY time DESC LIMIT 1',[device]),
            db.queryOne(`SELECT ROUND(MIN(temperature)::numeric,1) tmin,ROUND(MAX(temperature)::numeric,1) tmax,
                ROUND(AVG(temperature)::numeric,1) tavg,COUNT(*) readings
                FROM sensor_readings WHERE device_id=$1 AND time>NOW()-INTERVAL '24 hours'`,[device]),
            db.query('SELECT * FROM actuator_events WHERE device_id=$1 ORDER BY time DESC LIMIT 10',[device]),
            db.query('SELECT * FROM alert_history WHERE device_id=$1 AND acknowledged=false ORDER BY time DESC LIMIT 5',[device]),
            db.queryOne('SELECT * FROM devices WHERE device_id=$1',[device]),
        ]);
        const online = dev?.last_seen && (Date.now()-new Date(dev.last_seen).getTime())<120000;
        res.json({device:{...dev,online},latest,stats_24h:stats,recent_events:events,active_alerts:alerts,ts:new Date()});
    } catch(e) { res.status(500).json({error:e.message}); }
});
module.exports = router;
