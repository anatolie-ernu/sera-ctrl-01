'use strict';
const router = require('express').Router();
const db     = require('../../db/pool');
const {auth,adminOnly} = require('../middleware/auth');
router.get('/rules', auth, async (req,res) => res.json({data: await db.query('SELECT * FROM alert_rules ORDER BY id')}));
router.post('/rules', auth, adminOnly, async (req, res) => {
    const {device_id='SERA_001',name,parameter,operator,threshold,severity='warning',notify_email=true,notify_sms=false,cooldown_min=30} = req.body;
    const r = await db.query('INSERT INTO alert_rules(device_id,name,parameter,operator,threshold,severity,notify_email,notify_sms,cooldown_min) VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9) RETURNING id',
        [device_id,name,parameter,operator,threshold,severity,notify_email,notify_sms,cooldown_min]);
    res.status(201).json({id:r[0].id});
});
router.put('/rules/:id', auth, adminOnly, async (req,res) => {
    const {threshold,enabled,notify_email,notify_sms,cooldown_min} = req.body;
    await db.query('UPDATE alert_rules SET threshold=COALESCE($1,threshold),enabled=COALESCE($2,enabled),notify_email=COALESCE($3,notify_email),notify_sms=COALESCE($4,notify_sms),cooldown_min=COALESCE($5,cooldown_min) WHERE id=$6',
        [threshold??null,enabled??null,notify_email??null,notify_sms??null,cooldown_min??null,req.params.id]);
    res.json({message:'Actualizat'});
});
router.get('/history', auth, async (req,res) => {
    const {limit=50,acknowledged} = req.query;
    let sql='SELECT ah.*,ar.name rule_name FROM alert_history ah LEFT JOIN alert_rules ar ON ah.rule_id=ar.id';
    const p=[];
    if(acknowledged!==undefined){sql+=' WHERE ah.acknowledged=$1';p.push(acknowledged==='true');}
    sql+=` ORDER BY ah.time DESC LIMIT ${parseInt(limit)}`;
    res.json({data: await db.query(sql,p)});
});
router.put('/history/:id/ack', auth, async (req,res) => {
    await db.query('UPDATE alert_history SET acknowledged=true,ack_by=$1,ack_at=NOW() WHERE ctid=(SELECT ctid FROM alert_history WHERE time=$2 LIMIT 1)',[req.user.id,req.params.id])
        .catch(()=>db.query('UPDATE alert_history SET acknowledged=true,ack_by=$1,ack_at=NOW() WHERE id=$2',[req.user.id,req.params.id]));
    res.json({message:'Confirmat'});
});
module.exports = router;
