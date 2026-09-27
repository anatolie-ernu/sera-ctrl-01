/**
 * db/pool.js — Pool conexiuni PostgreSQL/TimescaleDB
 */
'use strict';
const { Pool } = require('pg');
const logger   = require('../utils/logger');

const pool = new Pool({
    host:     process.env.DB_HOST || 'localhost',
    port:     parseInt(process.env.DB_PORT || '5432'),
    database: process.env.DB_NAME || 'sera_db',
    user:     process.env.DB_USER || 'sera_user',
    password: process.env.DB_PASS || '',
    max: 20, idleTimeoutMillis: 30000, connectionTimeoutMillis: 5000,
});

pool.on('error', (err) => logger.error('Pool PostgreSQL eroare:', err.message));

async function connect() {
    const c = await pool.connect();
    logger.info(`TimescaleDB conectat: ${process.env.DB_HOST}:${process.env.DB_PORT}/${process.env.DB_NAME}`);
    c.release();
}

async function query(text, params = []) {
    const start = Date.now();
    try {
        const { rows } = await pool.query(text, params);
        const dur = Date.now() - start;
        if (dur > 1000) logger.warn(`Query lenta (${dur}ms): ${text.substring(0,80)}`);
        return rows;
    } catch (err) {
        logger.error('Query eroare:', { sql: text.substring(0,80), error: err.message });
        throw err;
    }
}

async function queryOne(text, params = []) {
    const rows = await query(text, params);
    return rows[0] || null;
}

async function transaction(fn) {
    const client = await pool.connect();
    try {
        await client.query('BEGIN');
        const result = await fn(client);
        await client.query('COMMIT');
        return result;
    } catch (err) {
        await client.query('ROLLBACK');
        throw err;
    } finally {
        client.release();
    }
}

async function close() { await pool.end(); logger.info('Pool TimescaleDB inchis'); }

module.exports = { connect, query, queryOne, transaction, close };
