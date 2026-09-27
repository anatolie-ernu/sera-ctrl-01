/**
 * scheduler/irrigation.js  —  Greenhouse IoT System | Backend
 * ─────────────────────────────────────────────────────────────────────────────
 * Runs cron-based irrigation programs defined in the irrigation_schedules table.
 *
 * Dynamic schedule loading
 * ─────────────────────────────────────────────────────────────────────────────
 * Rather than requiring a server restart when schedules are added or modified
 * via the API, the scheduler reloads the active schedule list every 5 minutes.
 * This means:
 *   • New schedules are picked up within 5 minutes.
 *   • Disabled/deleted schedules stop running within 5 minutes.
 *   • Cron expressions can be changed without downtime.
 *
 * Execution flow
 * ─────────────────────────────────────────────────────────────────────────────
 * When a schedule's cron expression fires:
 *   1. Insert a row into irrigation_log with started_at = NOW().
 *   2. Publish MQTT command {action:"ON", duration_ms: N} to the pump topic.
 *   3. Update irrigation_schedules.last_run and increment run_count.
 *   4. await sleep(duration + 2 s) — the ESP32 stops the pump via its timer,
 *      but we wait server-side to mark the log entry as complete.
 *   5. Update irrigation_log.success = true.
 *
 * NOTE: step 4 blocks the async function for the duration of the watering
 * cycle.  This is acceptable because the function runs in its own isolated
 * async context; other requests continue to be served normally.
 */
'use strict';

const cron   = require('node-cron');
const db     = require('../db/pool');
const mqtt   = require('../mqtt/service');
const logger = require('../utils/logger');

// Map of active cron jobs: schedule.id → node-cron Task
const activeJobs = new Map();

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// ─────────────────────────────────────────────────────────────────────────────
/** Start the scheduler and set up the 5-minute reload job. */
async function start() {
    logger.info('[Scheduler] Starting irrigation scheduler...');
    await reload();

    // Reload schedule list every 5 minutes
    cron.schedule('*/5 * * * *', reload, { timezone: 'Europe/Bucharest' });
    logger.info('[Scheduler] Active');
}

function stop() {
    activeJobs.forEach((job) => job.stop());
    activeJobs.clear();
    logger.info('[Scheduler] Stopped');
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Reload active schedules from DB and reconcile with running cron jobs.
 * Called on startup and every 5 minutes.
 */
async function reload() {
    try {
        const schedules = await db.query(
            `SELECT * FROM irrigation_schedules WHERE enabled = true ORDER BY id`
        );

        const activeIds = new Set(schedules.map((s) => s.id));

        // Stop jobs that are no longer in the active list
        activeJobs.forEach((job, id) => {
            if (!activeIds.has(id)) {
                job.stop();
                activeJobs.delete(id);
                logger.info(`[Scheduler] Stopped job #${id} (disabled/deleted)`);
            }
        });

        // Start jobs for newly active schedules
        for (const s of schedules) {
            if (activeJobs.has(s.id)) continue;  // Already running

            if (!cron.validate(s.cron_expr)) {
                logger.warn(`[Scheduler] Invalid cron expression for schedule #${s.id}: "${s.cron_expr}"`);
                continue;
            }

            const job = cron.schedule(
                s.cron_expr,
                () => execute(s),
                { timezone: 'Europe/Bucharest' }
            );
            activeJobs.set(s.id, job);
            logger.info(`[Scheduler] Registered job #${s.id} "${s.name}": ${s.cron_expr}`);
        }

        logger.debug(`[Scheduler] ${activeJobs.size} active jobs`);
    } catch (err) {
        logger.error('[Scheduler] reload error:', err.message);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Execute one irrigation cycle.
 * @param {Object} s  A row from irrigation_schedules
 */
async function execute(s) {
    logger.info(`[Scheduler] Executing "${s.name}" — ${s.duration_s}s on ${s.device_id}`);

    let logId;
    try {
        // Record start in irrigation log
        const rows = await db.query(`
            INSERT INTO irrigation_log (time, schedule_id, device_id, duration_s, triggered_by)
            VALUES (NOW(), $1, $2, $3, 'schedule')
            RETURNING *`,
            [s.id, s.device_id, s.duration_s]
        );
        logId = rows[0]?.id;

        // Send pump ON command to ESP32 via MQTT
        const durationMs = s.duration_s * 1000;
        mqtt.sendCommand(s.device_id, 'pump', {
            action:      'ON',
            duration_ms: durationMs,
        });

        // Update schedule stats
        await db.query(`
            UPDATE irrigation_schedules
            SET last_run   = NOW(),
                run_count  = run_count + 1
            WHERE id = $1`,
            [s.id]
        );

        // Wait for the pump cycle to complete (plus a 2 s buffer)
        await sleep(durationMs + 2000);

        // Mark log entry as successful
        if (logId) {
            await db.query(
                `UPDATE irrigation_log SET success = true WHERE id = $1`,
                [logId]
            );
        }

        logger.info(`[Scheduler] "${s.name}" completed after ${s.duration_s}s`);
    } catch (err) {
        logger.error(`[Scheduler] execute error for schedule #${s.id}:`, err.message);
        // Record failure in log
        if (logId) {
            await db.query(
                `UPDATE irrigation_log SET success = false, notes = $1 WHERE id = $2`,
                [err.message.substring(0, 200), logId]
            ).catch(() => {});
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Trigger an immediate manual irrigation cycle outside of the cron schedule.
 * Called from POST /api/schedules/trigger.
 * @returns {number} The irrigation_log row ID for tracking
 */
async function triggerManual(deviceId, durationS, userId) {
    logger.info(`[Scheduler] Manual irrigation: ${deviceId} for ${durationS}s (user ${userId})`);

    const rows = await db.query(`
        INSERT INTO irrigation_log (time, device_id, duration_s, triggered_by)
        VALUES (NOW(), $1, $2, 'manual')
        RETURNING *`,
        [deviceId, durationS]
    );
    const logId = rows[0]?.id;

    // Fire pump command immediately
    mqtt.sendCommand(deviceId, 'pump', {
        action:      'ON',
        duration_ms: durationS * 1000,
    });

    // Close the log entry after the cycle completes (non-blocking — uses setTimeout)
    setTimeout(async () => {
        if (logId) {
            await db.query(
                `UPDATE irrigation_log SET success = true WHERE id = $1`,
                [logId]
            ).catch(() => {});
        }
    }, durationS * 1000 + 2000);

    return logId;
}

module.exports = { start, stop, reload, triggerManual };
