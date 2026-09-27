'use strict';
const router = require('express').Router();
const db     = require('../../db/pool');
const mqtt   = require('../../mqtt/service');
const {auth} = require('../middleware/auth');
router.post('/command', auth, async (req, res) => {
    const {device_id='SERA_001',type,action,params={}} = req.body;
    if(!type||!action) return res.status(400).json({error:'type si action obligatorii'});
    const ok = mqtt.sendCommand(device_id, type, {action,...params});
    await db.query('INSERT INTO actuator_events(time,device_id,device_name,action,triggered_by,user_id) VALUES(NOW(),$1,$2,$3,$4,$5)',
        [device_id,type,action,'manual',req.user.id]).catch(()=>{});
    res.json({success:ok,message:ok?'Comanda trimisa':'MQTT neconectat'});
});
router.get('/events', auth, async (req, res) => {
    const {device='SERA_001',limit=100} = req.query;
    const rows = await db.query(`SELECT ae.*,u.username FROM actuator_events ae
        LEFT JOIN users u ON ae.user_id=u.id WHERE ae.device_id=$1 ORDER BY ae.time DESC LIMIT $2`,
        [device,parseInt(limit)]);
    res.json({data:rows});
});
module.exports = router;
