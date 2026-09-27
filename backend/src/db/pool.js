/**
 * db/pool.js  —  Greenhouse IoT System | Backend
 * ─────────────────────────────────────────────────────────────────────────────
 * PostgreSQL / TimescaleDB connection pool built on the `pg` library.
 *
 * Why a connection pool?
 * ─────────────────────────────────────────────────────────────────────────────
 * Each database operation (INSERT sensor reading, SELECT history, etc.) needs
 * a connection to PostgreSQL.  Opening a new TCP connection for every operation
 * is expensive (~50 ms) and limits throughput.  A pool maintains a set of
 * reusable connections and hands them out on demand, returning them to the pool
 * when the operation completes.
 *
 * The pool is configured with:
 *   max: 20      — up to 20 simultaneous connections (tune based on pg max_connections)
 *   idleTimeoutMillis: 30000  — idle connections are released after 30 s
 *   connectionTimeoutMillis: 5000 — fail fast if a connection cannot be acquired
 *
 * Helper functions
 * ─────────────────────────────────────────────────────────────────────────────
 * query(text, params)     — execute a parameterised query, return all rows
 * queryOne(text, params)  — same but return the first row or null
 * transaction(fn)         — run fn(client) inside a BEGIN/COMMIT block;
 *                           automatically rolls back on any thrown error
 */
'use strict';

const { Pool } = require('pg');
const logger   = require('../utils/logger');

// ── Pool configuration from environment variables ────────────────────────────
// These are set in .env and passed to the container via docker-compose.yml.
// The defaults work for local development without Docker.
const pool = new Pool({
    host:                    process.env.DB_HOST     || 'localhost',
    port:                    parseInt(process.env.DB_PORT  || '5432'),
    database:                process.env.DB_NAME     || 'sera_db',
    user:                    process.env.DB_USER     || 'sera_user',
    password:                process.env.DB_PASS     || '',
    max:                     20,              // Maximum concurrent connections
    idleTimeoutMillis:       30_000,          // Release idle connections after 30 s
    connectionTimeoutMillis: 5_000,           // Fail if no connection available in 5 s
});

// Log pool-level errors (e.g. database server restart) without crashing the app
pool.on('error', (err) => {
    logger.error('Unexpected PostgreSQL pool error:', err.message);
});

// ─────────────────────────────────────────────────────────────────────────────
/** Test the connection at startup and log the server details. */
async function connect() {
    const client = await pool.connect();
    const res = await client.query('SELECT version()');
    logger.info(`TimescaleDB connected: ${process.env.DB_HOST}:${process.env.DB_PORT}/${process.env.DB_NAME}`);
    logger.debug(`PostgreSQL: ${res.rows[0].version.split(' ').slice(0,2).join(' ')}`);
    client.release();
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Execute a parameterised SQL query and return all matching rows.
 *
 * @param  {string}  text    SQL string with $1, $2, ... placeholders
 * @param  {Array}   params  Values to bind to the placeholders
 * @returns {Array<Object>}  Array of row objects (empty array if no results)
 *
 * Security: always use parameterised queries — never concatenate user input
 * into the SQL string, as that would create an SQL injection vulnerability.
 *
 * Performance: queries taking longer than 1 s are logged as warnings so slow
 * queries can be identified and optimised.
 */
async function query(text, params = []) {
    const start = Date.now();
    try {
        const { rows } = await pool.query(text, params);
        const elapsed  = Date.now() - start;
        if (elapsed > 1000) {
            logger.warn(`Slow query (${elapsed} ms): ${text.substring(0, 120)}`);
        }
        return rows;
    } catch (err) {
        logger.error('Query error:', {
            sql:   text.substring(0, 120),
            error: err.message,
        });
        throw err;  // Re-throw so route handlers can return 500 to the client
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Execute a query and return only the first row, or null if no rows matched.
 * Useful for lookups where you expect at most one result.
 */
async function queryOne(text, params = []) {
    const rows = await query(text, params);
    return rows[0] || null;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Run a function inside a database transaction.
 *
 * The function receives the pg client as its first argument and should
 * use client.query() for all SQL within the transaction.
 *
 * If fn throws any error, the transaction is rolled back automatically.
 * If fn completes without throwing, the transaction is committed.
 *
 * @example
 * await transaction(async (client) => {
 *   await client.query('INSERT INTO ...', [...]);
 *   await client.query('UPDATE ...', [...]);
 *   // Both succeed or both are rolled back
 * });
 */
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
        client.release();  // Always return the connection to the pool
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/** Gracefully drain the pool (used during SIGTERM / SIGINT shutdown). */
async function close() {
    await pool.end();
    logger.info('PostgreSQL pool closed');
}

module.exports = { connect, query, queryOne, transaction, close };
