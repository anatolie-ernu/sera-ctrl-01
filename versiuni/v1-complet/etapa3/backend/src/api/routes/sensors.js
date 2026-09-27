'use strict';
const router = require('express').Router();
const db     = require('../../db/pool');
const {auth} = require('../middleware/auth');

// Ultima citire
router.get('/latest', auth, async (req, res) => {
    try {
        const rows = await db.query(`
            SELECT DISTINCT ON (device_id) *
            FROM sensor_readings ORDER BY device_id, time DESC`);
        res.json({data:rows});
    } catch(e) { res.status(500).json({error:e.message}); }
});

// Istoric cu agregare automata din continuous aggregates (RAPID!)
router.get('/history', auth, async (req, res) => {
    const {device='SERA_001', from, to, interval='5min'} = req.query;
    // Alege view-ul potrivit pe baza intervalului
    const viewMap = {
        '1min':'sensor_readings','5min':'sensor_5min',
        '15min':'sensor_5min','30min':'sensor_5min',
        '1h':'sensor_1h','6h':'sensor_1h','1d':'sensor_1d','7d':'sensor_1d'
    };
    const bucketMap = {
        '1min':'1 minute','5min':'5 minutes','15min':'15 minutes',
        '30min':'30 minutes','1h':'1 hour','6h':'6 hours','1d':'1 day','7d':'7 days'
    };
    const view   = viewMap[interval]   || 'sensor_5min';
    const bucket = bucketMap[interval] || '5 minutes';
    const fromDt = from || new Date(Date.now()-86400000).toISOString();
    const toDt   = to   || new Date().toISOString();
    try {
        let rows;
        if (view === 'sensor_readings') {
            // Date brute pentru intervale mici
            rows = await db.query(`
                SELECT time_bucket($1::interval, time) AS bucket,
                    ROUND(AVG(temperature)::numeric,2) AS temp_avg,
                    ROUND(MIN(temperature)::numeric,2) AS temp_min,
                    ROUND(MAX(temperature)::numeric,2) AS temp_max,
                    ROUND(AVG(humidity)::numeric,2) AS hum_avg,
                    ROUND(AVG(soil_pct)::numeric,2) AS soil_avg, COUNT(*) AS readings
                FROM sensor_readings
                WHERE device_id=$2 AND time BETWEEN $3 AND $4 AND dht_valid=true
                GROUP BY bucket ORDER BY bucket`, [bucket,device,fromDt,toDt]);
        } else {
            // Continuous aggregate (pre-calculat, instant!)
            rows = await db.query(`
                SELECT bucket, temp_avg, temp_min, temp_max, hum_avg, soil_avg, readings
                FROM ${view}
                WHERE device_id=$1 AND bucket BETWEEN $2 AND $3
                ORDER BY bucket`, [device,fromDt,toDt]);
        }
        res.json({data:rows, count:rows.length, interval, view_used:view});
    } catch(e) { res.status(500).json({error:e.message}); }
});

// Statistici rapide
router.get('/stats', auth, async (req, res) => {
    const {device='SERA_001', period='24h'} = req.query;
    const h = {'24h':24,'7d':168,'30d':720}[period]||24;
    try {
        const row = await db.queryOne(`
            SELECT ROUND(MIN(temperature)::numeric,1) temp_min,
                   ROUND(MAX(temperature)::numeric,1) temp_max,
                   ROUND(AVG(temperature)::numeric,1) temp_avg,
                   ROUND(MIN(humidity)::numeric,1) hum_min,
                   ROUND(MAX(humidity)::numeric,1) hum_max,
                   ROUND(AVG(humidity)::numeric,1) hum_avg,
                   ROUND(MIN(soil_pct)::numeric,1) soil_min,
                   ROUND(MAX(soil_pct)::numeric,1) soil_max,
                   ROUND(AVG(soil_pct)::numeric,1) soil_avg,
                   COUNT(*) readings
            FROM sensor_readings
            WHERE device_id=$1 AND time>NOW()-($2||' hours')::interval AND dht_valid=true`,
            [device, h]);
        res.json({data:row, period, device});
    } catch(e) { res.status(500).json({error:e.message}); }
});

// Export CSV
router.get('/export', auth, async (req, res) => {
    const {device='SERA_001',from,to,format='csv'} = req.query;
    try {
        const rows = await db.query(`
            SELECT time,temperature,humidity,soil_raw,soil_pct
            FROM sensor_readings WHERE device_id=$1
              AND ($2::timestamptz IS NULL OR time>=$2)
              AND ($3::timestamptz IS NULL OR time<=$3)
            ORDER BY time LIMIT 100000`,
            [device,from||null,to||null]);
        if (format==='csv') {
            res.setHeader('Content-Type','text/csv');
            res.setHeader('Content-Disposition',`attachment;filename=sensors_${device}.csv`);
            res.send('time,temperature,humidity,soil_raw,soil_pct\n'+
                rows.map(r=>`${r.time},${r.temperature},${r.humidity},${r.soil_raw},${r.soil_pct}`).join('\n'));
        } else res.json({data:rows,count:rows.length});
    } catch(e) { res.status(500).json({error:e.message}); }
});
module.exports = router;
