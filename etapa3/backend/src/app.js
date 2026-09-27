'use strict';
const express      = require('express');
const cors         = require('cors');
const helmet       = require('helmet');
const rateLimit    = require('express-rate-limit');
const morgan       = require('morgan');
const path         = require('path');
const logger       = require('./utils/logger');
const db           = require('./db/pool');
const mqttSvc      = require('./mqtt/service');
const scheduler    = require('./scheduler/irrigation');
const alertEngine  = require('./alerts/engine');

const app = express();
app.use(helmet()); app.use(cors({origin:'*',credentials:true}));
app.use(express.json({limit:'1mb'}));
app.use(morgan('combined',{stream:{write:m=>logger.http(m.trim())}}));
app.use('/api/', rateLimit({windowMs:15*60*1000,max:300}));
app.use('/api/auth/login', rateLimit({windowMs:15*60*1000,max:10}));

app.use('/api/auth',      require('./api/routes/auth'));
app.use('/api/sensors',   require('./api/routes/sensors'));
app.use('/api/actuators', require('./api/routes/actuators'));
app.use('/api/schedules', require('./api/routes/schedules'));
app.use('/api/alerts',    require('./api/routes/alerts'));
app.use('/api/devices',   require('./api/routes/devices'));
app.use('/api/dashboard', require('./api/routes/dashboard'));
app.use(express.static(path.join(__dirname,'../public')));

app.get('/health', async (req,res) => {
    try {
        await db.query('SELECT 1');
        res.json({status:'ok',db:'connected',mqtt:mqttSvc.isConnected()?'connected':'disconnected',ts:new Date()});
    } catch(e) { res.status(503).json({status:'error',error:e.message}); }
});
app.use((req,res)=>res.status(404).json({error:`${req.method} ${req.path} inexistent`}));
app.use((err,req,res,_)=>{
    logger.error('Eroare:', err.message);
    res.status(err.status||500).json({error:process.env.NODE_ENV==='production'?'Eroare server':err.message});
});

async function start() {
    try {
        await db.connect();
        await mqttSvc.connect();
        await scheduler.start();
        alertEngine.start();
        const PORT=process.env.PORT||3000;
        app.listen(PORT,'0.0.0.0',()=>logger.info(`Backend pornit pe :${PORT}`));
    } catch(e) { logger.error('Start fail:',e); process.exit(1); }
}
['SIGTERM','SIGINT'].forEach(s=>process.on(s,async()=>{
    logger.info(`${s} — oprire gradata`);
    scheduler.stop(); alertEngine.stop();
    await mqttSvc.disconnect(); await db.close(); process.exit(0);
}));
start();
module.exports = app;
