'use strict';
const winston = require('winston');
const path    = require('path');
const logDir  = process.env.LOG_DIR || path.join(__dirname,'../../logs');
require('fs').mkdirSync(logDir, {recursive:true});

const fmt = winston.format;
module.exports = winston.createLogger({
    level: process.env.LOG_LEVEL || 'info',
    format: fmt.combine(fmt.timestamp({format:'YYYY-MM-DD HH:mm:ss'}), fmt.errors({stack:true}), fmt.json()),
    transports: [
        new winston.transports.Console({
            format: fmt.combine(fmt.colorize(), fmt.printf(
                ({timestamp,level,message,...m}) =>
                `${timestamp} [${level}] ${message}${Object.keys(m).length?' '+JSON.stringify(m):''}`
            ))
        }),
        new winston.transports.File({filename:path.join(logDir,'error.log'),level:'error',maxsize:10485760,maxFiles:5}),
        new winston.transports.File({filename:path.join(logDir,'app.log'),maxsize:52428800,maxFiles:10}),
    ],
});
