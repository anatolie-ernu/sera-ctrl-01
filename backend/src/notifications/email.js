'use strict';
const nodemailer = require('nodemailer');
const logger     = require('../utils/logger');
let tr;
function getTr() {
    if(!tr) tr=nodemailer.createTransporter({
        host:process.env.SMTP_HOST,port:parseInt(process.env.SMTP_PORT||587),
        secure:process.env.SMTP_PORT==='465',
        auth:{user:process.env.SMTP_USER,pass:process.env.SMTP_PASS}});
    return tr;
}
const COLORS={info:'#17a2b8',warning:'#ffc107',critical:'#dc3545'};
async function sendAlert(to,severity,message,details={}) {
    const c=COLORS[severity]||'#333';
    const html=`<div style="font-family:Arial;max-width:600px">
<div style="background:${c};color:white;padding:16px;border-radius:8px 8px 0 0"><h2 style="margin:0">🌿 Sera — Alerta ${severity.toUpperCase()}</h2></div>
<div style="background:#f8f9fa;padding:16px;border:1px solid #ddd;border-radius:0 0 8px 8px">
<p><strong>${message}</strong></p>
<table style="width:100%">
<tr><td><b>Dispozitiv</b></td><td>${details.device||''}</td></tr>
<tr><td><b>Parametru</b></td><td>${details.parameter||''}</td></tr>
<tr><td><b>Valoare</b></td><td style="color:${c};font-weight:bold">${details.value??''}</td></tr>
<tr><td><b>Prag</b></td><td>${details.threshold??''}</td></tr>
</table>
<p style="color:#666;font-size:12px">${new Date().toLocaleString('ro-RO',{timeZone:'Europe/Bucharest'})}</p>
</div></div>`;
    await getTr().sendMail({from:process.env.SMTP_FROM,to,subject:`🌡️ Sera [${severity.toUpperCase()}]: ${details.parameter||message.substring(0,50)}`,html,text:message});
    logger.info(`Email trimis: ${to}`);
}
module.exports = {sendAlert};
