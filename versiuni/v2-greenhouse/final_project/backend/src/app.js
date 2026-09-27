/**
 * app.js  —  Greenhouse IoT System | Backend
 * ─────────────────────────────────────────────────────────────────────────────
 * Express application entry point.
 *
 * Startup sequence
 * ─────────────────────────────────────────────────────────────────────────────
 * 1. Create Express app and apply global middleware.
 * 2. Mount all API route handlers.
 * 3. Connect to TimescaleDB (pool.connect).
 * 4. Connect to Mosquitto MQTT broker (mqttService.connect).
 * 5. Start the irrigation scheduler (cron jobs).
 * 6. Start the alert engine.
 * 7. Begin accepting HTTP connections.
 *
 * Graceful shutdown
 * ─────────────────────────────────────────────────────────────────────────────
 * SIGTERM (sent by Docker on 'docker stop') and SIGINT (Ctrl+C) trigger
 * an ordered shutdown: stop cron jobs, disconnect MQTT, drain DB pool.
 * This ensures in-flight database writes complete before the process exits.
 */
'use strict';

const express    = require('express');
const cors       = require('cors');
const helmet     = require('helmet');
const rateLimit  = require('express-rate-limit');
const morgan     = require('morgan');
const path       = require('path');

const logger     = require('./utils/logger');
const db         = require('./db/pool');
const mqttSvc    = require('./mqtt/service');
const scheduler  = require('./scheduler/irrigation');
const alertEng   = require('./alerts/engine');

const app = express();

// ── Security middleware ───────────────────────────────────────────────────────
// helmet sets security HTTP headers:
//   X-Frame-Options: SAMEORIGIN       — prevents clickjacking
//   X-Content-Type-Options: nosniff   — prevents MIME sniffing
//   Strict-Transport-Security         — enforces HTTPS (via Nginx)
app.use(helmet());

// CORS: allow all origins for development; restrict in production via CORS_ORIGIN env var
app.use(cors({ origin: process.env.CORS_ORIGIN || '*', credentials: true }));

// Parse JSON request bodies up to 1 MB
app.use(express.json({ limit: '1mb' }));
app.use(express.urlencoded({ extended: true }));

// HTTP request logging (writes to Winston logger stream)
app.use(morgan('combined', { stream: { write: (m) => logger.http(m.trim()) } }));

// ── Rate limiting ─────────────────────────────────────────────────────────────
// Global limit: 300 requests per 15 minutes per IP (prevents API abuse)
app.use('/api/', rateLimit({
    windowMs: 15 * 60 * 1000,
    max:      300,
    standardHeaders: true,
    message: { error: 'Too many requests — please try again later' },
}));

// Strict limit on login endpoint to prevent brute-force password attacks
app.use('/api/auth/login', rateLimit({
    windowMs: 15 * 60 * 1000,
    max:      10,
    message: { error: 'Too many login attempts — please wait 15 minutes' },
}));

// ── API routes ────────────────────────────────────────────────────────────────
// Each route module handles one resource domain.
// All protected routes verify JWT via the auth middleware inside each router.
app.use('/api/auth',      require('./api/routes/auth'));
app.use('/api/sensors',   require('./api/routes/sensors'));
app.use('/api/actuators', require('./api/routes/actuators'));
app.use('/api/schedules', require('./api/routes/schedules'));
app.use('/api/alerts',    require('./api/routes/alerts'));
app.use('/api/devices',   require('./api/routes/devices'));
app.use('/api/dashboard', require('./api/routes/dashboard'));

// ── Static files (React frontend build) ──────────────────────────────────────
// When the frontend is built with 'npm run build', the output goes into
// backend/public/.  Express serves it here so one Docker container handles
// both API and frontend.
app.use(express.static(path.join(__dirname, '../public')));

// ── Health check ──────────────────────────────────────────────────────────────
// Used by Docker HEALTHCHECK and external monitoring (Uptime Kuma, etc.)
// Returns 200 if DB and MQTT are connected; 503 otherwise.
app.get('/health', async (req, res) => {
    try {
        await db.query('SELECT 1');
        res.json({
            status:    'ok',
            timestamp: new Date().toISOString(),
            db:        'connected',
            mqtt:      mqttSvc.isConnected() ? 'connected' : 'disconnected',
        });
    } catch (err) {
        res.status(503).json({ status: 'error', db: 'disconnected', error: err.message });
    }
});

// ── 404 handler ───────────────────────────────────────────────────────────────
app.use((req, res) => {
    res.status(404).json({ error: `Route ${req.method} ${req.path} not found` });
});

// ── Global error handler ──────────────────────────────────────────────────────
// Catches any error thrown by route handlers.
// In production, hides internal details; in development, shows the message.
app.use((err, req, res, _next) => {
    logger.error('Unhandled error:', { message: err.message, stack: err.stack });
    res.status(err.status || 500).json({
        error: process.env.NODE_ENV === 'production'
            ? 'Internal server error'
            : err.message,
    });
});

// ── Startup ───────────────────────────────────────────────────────────────────
async function start() {
    try {
        logger.info('Connecting to TimescaleDB...');
        await db.connect();

        logger.info('Connecting to MQTT broker...');
        await mqttSvc.connect();

        logger.info('Starting irrigation scheduler...');
        await scheduler.start();

        logger.info('Starting alert engine...');
        alertEng.start();

        const PORT = process.env.PORT || 3000;
        app.listen(PORT, '0.0.0.0', () => {
            logger.info(`Backend listening on port ${PORT}`);
            logger.info(`Health: http://localhost:${PORT}/health`);
        });
    } catch (err) {
        logger.error('Startup failed:', err);
        process.exit(1);
    }
}

// ── Graceful shutdown ─────────────────────────────────────────────────────────
// Docker sends SIGTERM when stopping a container.
// We drain in-flight work before exiting so data is not lost.
async function shutdown(signal) {
    logger.info(`${signal} received — graceful shutdown`);
    scheduler.stop();
    alertEng.stop();
    await mqttSvc.disconnect();
    await db.close();
    process.exit(0);
}

process.on('SIGTERM', () => shutdown('SIGTERM'));
process.on('SIGINT',  () => shutdown('SIGINT'));

start();

module.exports = app;  // Exported for use by Jest test suite
