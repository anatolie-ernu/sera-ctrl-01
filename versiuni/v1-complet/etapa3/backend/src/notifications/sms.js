'use strict';
const logger = require('../utils/logger');
async function send(to, message) {
    const prov=process.env.SMS_PROVIDER||'twilio';
    const msg=message.substring(0,160);
    logger.info(`SMS [${prov}] -> ${to}: ${msg}`);
    if(prov==='twilio') {
        const twilio=require('twilio')(process.env.SMS_ACCOUNT_SID,process.env.SMS_API_KEY);
        await twilio.messages.create({body:msg,from:process.env.SMS_FROM,to});
    } else if(prov==='smsapi') {
        const axios=require('axios');
        await axios.post('https://api.smsapi.com/sms.do',
            {access_token:process.env.SMS_API_KEY,to:to.replace('+',''),message:msg,format:'json'});
    } else logger.warn(`Provider SMS necunoscut: ${prov}`);
}
module.exports = {send};
