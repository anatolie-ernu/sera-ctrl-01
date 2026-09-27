'use strict';
const router    = require('express').Router();
const db        = require('../../db/pool');
const scheduler = require('../../scheduler/irrigation');
const {auth,adminOnly} = require('../middleware/auth');
router.get('/', auth, async (req, res) => {
    res.json({data: await db.query('SELECT * FROM irrigation_schedules ORDER BY id')});
});
router.post('/', auth, adminOnly, async (req, res) => {
    const {device_id='SERA_001',name,cron_expr,duration_s,enabled=true} = req.body;
    if(!name||!cron_expr||!duration_s) return res.status(400).json({error:'Campuri lipsa'});
    const r = await db.query(
        'INSERT INTO irrigation_schedules(device_id,name,cron_expr,duration_s,enabled,created_by) VALUES($1,$2,$3,$4,$5,$6) RETURNING id',
        [device_id,name,cron_expr,duration_s,enabled,req.user.id]);
    await scheduler.reload();
    res.status(201).json({id:r[0].id});
});
router.put('/:id', auth, adminOnly, async (req, res) => {
    const {name,cron_expr,duration_s,enabled} = req.body;
    await db.query(`UPDATE irrigation_schedules SET
        name=COALESCE($1,name),cron_expr=COALESCE($2,cron_expr),
        duration_s=COALESCE($3,duration_s),enabled=COALESCE($4,enabled),
        updated_at=NOW() WHERE id=$5`,
        [name||null,cron_expr||null,duration_s||null,enabled??null,req.params.id]);
    await scheduler.reload();
    res.json({message:'Actualizat'});
});
router.delete('/:id', auth, adminOnly, async (req, res) => {
    await db.query('UPDATE irrigation_schedules SET enabled=false WHERE id=$1',[req.params.id]);
    await scheduler.reload();
    res.json({message:'Dezactivat'});
});
router.post('/trigger', auth, async (req, res) => {
    const {device_id='SERA_001',duration_s=60} = req.body;
    const logId = await scheduler.triggerManual(device_id,duration_s,req.user.id);
    res.json({message:`Irigare ${duration_s}s pornita`,log_id:logId});
});
router.get('/log', auth, async (req, res) => {
    const rows = await db.query(`SELECT il.*,is2.name schedule_name FROM irrigation_log il
        LEFT JOIN irrigation_schedules is2 ON il.schedule_id=is2.id ORDER BY il.time DESC LIMIT 200`);
    res.json({data:rows});
});
module.exports = router;
