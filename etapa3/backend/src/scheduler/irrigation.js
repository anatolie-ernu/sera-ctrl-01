'use strict';
const cron   = require('node-cron');
const db     = require('../db/pool');
const mqtt   = require('../mqtt/service');
const logger = require('../utils/logger');
const jobs   = new Map();
const sleep  = ms => new Promise(r=>setTimeout(r,ms));

async function start() {
    logger.info('[Scheduler] Pornire...');
    await reload();
    cron.schedule('*/5 * * * *', reload);
    logger.info('[Scheduler] Activ');
}

async function reload() {
    try {
        const schedules = await db.query('SELECT * FROM irrigation_schedules WHERE enabled=true');
        const ids = new Set(schedules.map(s=>s.id));
        jobs.forEach((job,id)=>{ if(!ids.has(id)){job.stop();jobs.delete(id);} });
        for (const s of schedules) {
            if(jobs.has(s.id)||!cron.validate(s.cron_expr)) continue;
            const job=cron.schedule(s.cron_expr,()=>execute(s),{timezone:'Europe/Bucharest'});
            jobs.set(s.id,job);
            logger.info(`[Scheduler] Job #${s.id} "${s.name}": ${s.cron_expr}`);
        }
    } catch(e) { logger.error('[Scheduler] reload:', e.message); }
}

async function execute(s) {
    logger.info(`[Scheduler] Irigare: "${s.name}" ${s.duration_s}s`);
    const logId = (await db.query(
        'INSERT INTO irrigation_log(time,schedule_id,device_id,duration_s,triggered_by) VALUES(NOW(),$1,$2,$3,$4) RETURNING id',
        [s.id,s.device_id,s.duration_s,'schedule']
    ).catch(()=>[{}]))[0]?.id;
    try {
        mqtt.sendCommand(s.device_id,'pump',{action:'ON',duration_ms:s.duration_s*1000});
        await db.query('UPDATE irrigation_schedules SET last_run=NOW(),run_count=run_count+1 WHERE id=$1',[s.id]);
        await sleep(s.duration_s*1000+2000);
        if(logId) await db.query('UPDATE irrigation_log SET success=true WHERE id=$1',[logId]).catch(()=>{});
        logger.info(`[Scheduler] "${s.name}" finalizata`);
    } catch(e) {
        logger.error('[Scheduler] execute:', e.message);
        if(logId) await db.query('UPDATE irrigation_log SET success=false,notes=$1 WHERE id=$2',[e.message.substring(0,200),logId]).catch(()=>{});
    }
}

async function triggerManual(deviceId,durationS,userId) {
    const res=await db.query('INSERT INTO irrigation_log(time,device_id,duration_s,triggered_by) VALUES(NOW(),$1,$2,$3) RETURNING id',[deviceId,durationS,'manual']);
    const logId=res[0]?.id;
    mqtt.sendCommand(deviceId,'pump',{action:'ON',duration_ms:durationS*1000});
    setTimeout(()=>db.query('UPDATE irrigation_log SET success=true WHERE id=$1',[logId]).catch(()=>{}), durationS*1000+2000);
    return logId;
}

function stop() { jobs.forEach(j=>j.stop()); jobs.clear(); }
module.exports = {start,stop,reload,triggerManual};
