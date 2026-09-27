/**
 * alerts/engine.js  —  Greenhouse IoT System | Backend
 * ─────────────────────────────────────────────────────────────────────────────
 * Evaluates configurable threshold rules against incoming sensor readings and
 * dispatches email and/or SMS notifications when conditions are violated.
 *
 * Architecture
 * ─────────────────────────────────────────────────────────────────────────────
 * The engine runs in-process (no separate service) and is called synchronously
 * from the MQTT service handler after every sensor reading is saved.  This
 * ensures notifications are sent within the same event-loop cycle as the data
 * arrives — typically within a few hundred milliseconds of the ESP32 publishing.
 *
 * Cooldown mechanism
 * ─────────────────────────────────────────────────────────────────────────────
 * Each alert rule has a cooldown_min field (default: 30 minutes).  If a rule
 * fires, it cannot fire again until cooldown_min minutes have elapsed.
 *
 * The cooldown state is maintained in two places:
 *   1. In-memory Map (cooldowns) — checked first for speed (no DB round-trip)
 *   2. The alert_history table — provides persistence across server restarts
 *
 * The in-memory map is reset when the server restarts, but alert_history is
 * permanent.  For a fully persistent cooldown, query alert_history in
 * evaluateAlerts() before checking the map (left as an exercise).
 *
 * Notification delivery
 * ─────────────────────────────────────────────────────────────────────────────
 * Each alert is delivered to every user who has the corresponding
 * notify_email or notify_sms flag set.  This allows different users to
 * receive different notification types (e.g. one admin gets SMS + email,
 * another user gets only email).
 */
'use strict';

const db     = require('../db/pool');
const mailer = require('../notifications/email');
const sms    = require('../notifications/sms');
const logger = require('../utils/logger');

// In-memory cooldown tracker: Map<ruleId, lastFiredTimestampMs>
const cooldowns = new Map();

let _running = false;

function start() { _running = true;  logger.info('[AlertEngine] Started'); }
function stop()  { _running = false; logger.info('[AlertEngine] Stopped'); }

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Evaluate all enabled alert rules for the given device against the
 * latest sensor readings.
 *
 * @param {string} deviceId  Device identifier (e.g. "SERA_001")
 * @param {Object} readings  { temperature: number, humidity: number, soil_pct: number }
 */
async function evaluateAlerts(deviceId, readings) {
    if (!_running) return;

    try {
        const rules = await db.query(
            `SELECT * FROM alert_rules WHERE device_id = $1 AND enabled = true`,
            [deviceId]
        );

        for (const rule of rules) {
            // Get the sensor value for this rule's parameter
            const value = readings[rule.parameter];
            if (value == null || isNaN(value)) continue;  // Skip if sensor invalid

            // Evaluate the threshold condition
            if (!checkCondition(value, rule.operator, parseFloat(rule.threshold))) continue;

            // Check in-memory cooldown
            const key        = String(rule.id);
            const cooldownMs = rule.cooldown_min * 60_000;
            const lastFired  = cooldowns.get(key) || 0;
            if (Date.now() - lastFired < cooldownMs) {
                logger.debug(`Alert rule #${rule.id} in cooldown — skipping`);
                continue;
            }

            // Fire the alert
            await fireAlert(deviceId, rule, value);
            cooldowns.set(key, Date.now());
        }
    } catch (err) {
        logger.error('[AlertEngine] evaluateAlerts error:', err.message);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Evaluate a single condition: does `value op threshold` hold?
 * @param {number} value     Current sensor reading
 * @param {string} op        Operator string: '>', '<', '>=', '<=', '='
 * @param {number} threshold Configured threshold value
 * @returns {boolean}
 */
function checkCondition(value, op, threshold) {
    switch (op) {
        case '>':  return value >  threshold;
        case '<':  return value <  threshold;
        case '>=': return value >= threshold;
        case '<=': return value <= threshold;
        case '=':  return value === threshold;
        default:
            logger.warn(`Unknown operator: ${op}`);
            return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Record the alert in the database and dispatch configured notifications.
 * Failures in email or SMS delivery are logged but do not throw — one broken
 * notification channel should not prevent the other from being attempted.
 */
async function fireAlert(deviceId, rule, value) {
    const message = buildMessage(deviceId, rule, value);
    logger.warn(`[ALERT] ${message}`);

    // ── Persist to alert_history ──────────────────────────────────────────────
    let historyId;
    try {
        const rows = await db.query(`
            INSERT INTO alert_history
              (time, rule_id, device_id, parameter, value, threshold, message, severity)
            VALUES (NOW(), $1, $2, $3, $4, $5, $6, $7)
            RETURNING ctid`,  // TimescaleDB hypertables don't have a simple serial id
            [rule.id, deviceId, rule.parameter, value, rule.threshold, message, rule.severity]
        );
        historyId = rows[0]?.ctid;
    } catch (err) {
        logger.error('[AlertEngine] Failed to persist alert:', err.message);
        return;  // Don't send notifications if we can't record the alert
    }

    // ── Notify users ──────────────────────────────────────────────────────────
    const users = await db.query(
        `SELECT * FROM users WHERE notify_email = true OR notify_sms = true`
    ).catch(() => []);

    let emailSent = false;
    let smsSent   = false;

    for (const user of users) {
        // Email notification
        if (rule.notify_email && user.notify_email && user.email) {
            try {
                await mailer.sendAlert(user.email, rule.severity, message, {
                    parameter: rule.parameter,
                    value,
                    threshold: rule.threshold,
                    device:    deviceId,
                });
                emailSent = true;
            } catch (e) {
                logger.error(`Email failed to ${user.email}:`, e.message);
            }
        }

        // SMS notification (first 160 characters only — standard SMS limit)
        if (rule.notify_sms && user.notify_sms && user.phone) {
            try {
                await sms.send(user.phone, message.substring(0, 160));
                smsSent = true;
            } catch (e) {
                logger.error(`SMS failed to ${user.phone}:`, e.message);
            }
        }
    }

    // Update the alert history record with delivery status
    if (historyId) {
        await db.query(
            `UPDATE alert_history SET email_sent = $1, sms_sent = $2 WHERE ctid = $3`,
            [emailSent, smsSent, historyId]
        ).catch(() => {});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/** Build a human-readable alert message suitable for email subject and SMS. */
function buildMessage(deviceId, rule, value) {
    const labels = {
        temperature: `Temperature ${value.toFixed(1)}°C (threshold: ${rule.threshold}°C)`,
        humidity:    `Air humidity ${value.toFixed(1)}% (threshold: ${rule.threshold}%)`,
        soil_pct:    `Soil moisture ${value.toFixed(1)}% (threshold: ${rule.threshold}%)`,
    };
    const detail = labels[rule.parameter] || `${rule.parameter}=${value} threshold=${rule.threshold}`;
    return `[${rule.severity.toUpperCase()}] ${deviceId}: ${rule.name} — ${detail}`;
}

module.exports = { start, stop, evaluateAlerts };
