/**
 * api/routes/sensors.js  —  Greenhouse IoT System | Backend
 * ─────────────────────────────────────────────────────────────────────────────
 * REST endpoints for reading sensor history and statistics.
 *
 * Performance strategy: aggregate view routing
 * ─────────────────────────────────────────────────────────────────────────────
 * TimescaleDB pre-computes three continuous aggregate views:
 *   sensor_5min  — averages bucketed into 5-minute intervals
 *   sensor_1h    — averages bucketed into 1-hour intervals
 *   sensor_1d    — averages bucketed into 1-day intervals
 *
 * These views are refreshed automatically by background jobs and stored on
 * disk.  Querying them is essentially a sequential scan of a small, sorted
 * table — orders of magnitude faster than aggregating millions of raw rows.
 *
 * The /history endpoint maps the requested `interval` parameter to the
 * most appropriate view so the dashboard always gets fast responses regardless
 * of how much historical data has accumulated.
 */
'use strict';

const router   = require('express').Router();
const db       = require('../../db/pool');
const { auth } = require('../middleware/auth');

// ─────────────────────────────────────────────────────────────────────────────
/**
 * GET /api/sensors/latest
 * Returns the single most recent row per device, useful for the dashboard's
 * "current conditions" card.
 *
 * DISTINCT ON is a PostgreSQL extension that returns one row per unique
 * device_id — the one with the highest `time` value (thanks to ORDER BY).
 */
router.get('/latest', auth, async (req, res) => {
    try {
        const rows = await db.query(`
            SELECT DISTINCT ON (device_id) *
            FROM sensor_readings
            ORDER BY device_id, time DESC`
        );
        res.json({ data: rows, count: rows.length });
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

// ─────────────────────────────────────────────────────────────────────────────
/**
 * GET /api/sensors/history
 * Returns time-bucketed sensor averages for chart rendering.
 *
 * Query parameters:
 *   device   — device identifier (default: "SERA_001")
 *   from     — ISO 8601 start timestamp (default: 24 hours ago)
 *   to       — ISO 8601 end timestamp (default: now)
 *   interval — bucket size: 1min, 5min, 15min, 30min, 1h, 6h, 1d, 7d
 *
 * Response includes `view_used` so the client knows whether it got raw data
 * or pre-aggregated data (useful for debugging performance).
 */
router.get('/history', auth, async (req, res) => {
    const {
        device   = 'SERA_001',
        from     = new Date(Date.now() - 86_400_000).toISOString(),
        to       = new Date().toISOString(),
        interval = '5min',
    } = req.query;

    // Map requested interval to the most efficient data source
    const viewMap = {
        '1min':  'sensor_readings',  // Raw data — only for short recent windows
        '5min':  'sensor_5min',      // Pre-aggregated 5-min buckets
        '15min': 'sensor_5min',
        '30min': 'sensor_5min',
        '1h':    'sensor_1h',        // Pre-aggregated 1-hour buckets
        '6h':    'sensor_1h',
        '1d':    'sensor_1d',        // Pre-aggregated 1-day buckets
        '7d':    'sensor_1d',
    };

    const bucketMap = {
        '1min':  '1 minute',  '5min':  '5 minutes', '15min': '15 minutes',
        '30min': '30 minutes','1h':    '1 hour',     '6h':    '6 hours',
        '1d':    '1 day',     '7d':    '7 days',
    };

    const view   = viewMap[interval]   || 'sensor_5min';
    const bucket = bucketMap[interval] || '5 minutes';

    try {
        let rows;

        if (view === 'sensor_readings') {
            // For raw data, use time_bucket() to aggregate on the fly.
            // This is only used for very short windows (e.g. last hour) where
            // the number of raw rows is small.
            rows = await db.query(`
                SELECT
                    time_bucket($1::interval, time) AS bucket,
                    ROUND(AVG(temperature)::numeric, 2)  AS temp_avg,
                    ROUND(MIN(temperature)::numeric, 2)  AS temp_min,
                    ROUND(MAX(temperature)::numeric, 2)  AS temp_max,
                    ROUND(AVG(humidity)::numeric, 2)     AS hum_avg,
                    ROUND(AVG(soil_pct)::numeric, 2)     AS soil_avg,
                    COUNT(*)                              AS readings
                FROM sensor_readings
                WHERE device_id = $2
                  AND time BETWEEN $3 AND $4
                  AND dht_valid = true
                GROUP BY bucket
                ORDER BY bucket ASC`,
                [bucket, device, from, to]
            );
        } else {
            // For pre-aggregated views, just scan the materialized view.
            // This query is near-instant regardless of historical data volume.
            rows = await db.query(`
                SELECT bucket, temp_avg, temp_min, temp_max, hum_avg, soil_avg, readings
                FROM ${view}
                WHERE device_id = $1
                  AND bucket BETWEEN $2 AND $3
                ORDER BY bucket ASC`,
                [device, from, to]
            );
        }

        res.json({
            data:      rows,
            count:     rows.length,
            interval,
            view_used: view,   // Transparency: tell the client which view was used
            device,
        });
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

// ─────────────────────────────────────────────────────────────────────────────
/**
 * GET /api/sensors/stats
 * Returns min/max/average statistics for a time period.
 * Used by the "stats cards" on the dashboard.
 */
router.get('/stats', auth, async (req, res) => {
    const { device = 'SERA_001', period = '24h' } = req.query;
    const hourMap = { '24h': 24, '7d': 168, '30d': 720 };
    const hours   = hourMap[period] || 24;

    try {
        const row = await db.queryOne(`
            SELECT
                ROUND(MIN(temperature)::numeric, 1) AS temp_min,
                ROUND(MAX(temperature)::numeric, 1) AS temp_max,
                ROUND(AVG(temperature)::numeric, 1) AS temp_avg,
                ROUND(MIN(humidity)::numeric, 1)    AS hum_min,
                ROUND(MAX(humidity)::numeric, 1)    AS hum_max,
                ROUND(AVG(humidity)::numeric, 1)    AS hum_avg,
                ROUND(MIN(soil_pct)::numeric, 1)    AS soil_min,
                ROUND(MAX(soil_pct)::numeric, 1)    AS soil_max,
                ROUND(AVG(soil_pct)::numeric, 1)    AS soil_avg,
                COUNT(*)                             AS total_readings
            FROM sensor_readings
            WHERE device_id  = $1
              AND time        > NOW() - ($2 || ' hours')::interval
              AND dht_valid   = true`,
            [device, hours]
        );
        res.json({ data: row, period, device });
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

// ─────────────────────────────────────────────────────────────────────────────
/**
 * GET /api/sensors/export
 * Downloads raw sensor data as CSV or JSON.
 * Capped at 100 000 rows to prevent memory exhaustion.
 */
router.get('/export', auth, async (req, res) => {
    const { device = 'SERA_001', from, to, format = 'csv' } = req.query;

    try {
        const rows = await db.query(`
            SELECT time, temperature, humidity, soil_raw, soil_pct
            FROM sensor_readings
            WHERE device_id = $1
              AND ($2::timestamptz IS NULL OR time >= $2)
              AND ($3::timestamptz IS NULL OR time <= $3)
            ORDER BY time ASC
            LIMIT 100000`,
            [device, from || null, to || null]
        );

        if (format === 'csv') {
            const header = 'time,temperature,humidity,soil_raw,soil_pct\n';
            const body   = rows
                .map((r) => `${r.time},${r.temperature},${r.humidity},${r.soil_raw},${r.soil_pct}`)
                .join('\n');
            res.setHeader('Content-Type', 'text/csv');
            res.setHeader('Content-Disposition', `attachment; filename="sensors_${device}.csv"`);
            return res.send(header + body);
        }

        res.json({ data: rows, count: rows.length });
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

module.exports = router;
