'use strict';
const db     = require('../db/pool');
const mailer = require('../notifications/email');
const sms    = require('../notifications/sms');
const logger = require('../utils/logger');

const cooldowns = new Map();
let _running = false;

function start() { _running = true; logger.info('[AlertEngine] Pornit'); }
function stop()  { _running = false; }

async function evaluateAlerts(deviceId, readings) {
    if (!_running) return;
    try {
        const rules = await db.query(
            'SELECT * FROM alert_rules WHERE device_id=$1 AND enabled=true', [deviceId]);
        for (const rule of rules) {
            const val = readings[rule.parameter];
            if (val == null) continue;
            if (!eval_cond(val, rule.operator, parseFloat(rule.threshold))) continue;
            const key = String(rule.id);
            const coolMs = rule.cooldown_min * 60000;
            if (Date.now() - (cooldowns.get(key)||0) < coolMs) continue;
            await fireAlert(deviceId, rule, val);
            cooldowns.set(key, Date.now());
        }
    } catch(e) { logger.error('[AlertEngine]', e.message); }
}

function eval_cond(v, op, t) {
    switch(op) {
        case '>':  return v >  t;
        case '<':  return v <  t;
        case '>=': return v >= t;
        case '<=': return v <= t;
        case '=':  return v === t;
        default:   return false;
    }
}

async function fireAlert(deviceId, rule, value) {
    const msg = `[${rule.severity.toUpperCase()}] ${deviceId}: ${rule.name} — valoare ${value} (prag: ${rule.threshold})`;
    logger.warn('[ALERTA]', msg);
    const res = await db.query(`
        INSERT INTO alert_history
          (time,rule_id,device_id,parameter,value,threshold,message,severity)
        VALUES (NOW(),$1,$2,$3,$4,$5,$6,$7) RETURNING id`,
        [rule.id,deviceId,rule.parameter,value,rule.threshold,msg,rule.severity]);
    const histId = res[0]?.id;
    const users = await db.query('SELECT * FROM users WHERE notify_email=true OR notify_sms=true');
    let emailOk=false, smsOk=false;
    for (const u of users) {
        if (rule.notify_email && u.notify_email && u.email) {
            try { await mailer.sendAlert(u.email,rule.severity,msg,{
                parameter:rule.parameter,value,threshold:rule.threshold,device:deviceId}); emailOk=true; }
            catch(e) { logger.error('Email:', e.message); }
        }
        if (rule.notify_sms && u.notify_sms && u.phone) {
            try { await sms.send(u.phone, msg.substring(0,160)); smsOk=true; }
            catch(e) { logger.error('SMS:', e.message); }
        }
    }
    if (histId) await db.query(
        'UPDATE alert_history SET email_sent=$1,sms_sent=$2 WHERE id=$3',
        [emailOk,smsOk,histId]).catch(()=>{});
}

module.exports = { start, stop, evaluateAlerts };
