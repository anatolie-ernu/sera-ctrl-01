-- =============================================================
-- SERA INTELIGENTA — Schema TimescaleDB
-- =============================================================
CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

CREATE TABLE users (
    id SERIAL PRIMARY KEY, username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE, password VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL DEFAULT 'viewer' CHECK (role IN ('admin','viewer')),
    phone VARCHAR(20), notify_email BOOLEAN DEFAULT true,
    notify_sms BOOLEAN DEFAULT false, created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE devices (
    device_id VARCHAR(50) PRIMARY KEY, name VARCHAR(100) DEFAULT 'Sera 1',
    location VARCHAR(200), firmware VARCHAR(20),
    last_seen TIMESTAMPTZ, online BOOLEAN DEFAULT false,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- ── HYPERTABLE senzori ─────────────────────────────────────
CREATE TABLE sensor_readings (
    time TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    device_id VARCHAR(50) NOT NULL REFERENCES devices(device_id),
    temperature NUMERIC(5,2), humidity NUMERIC(5,2),
    soil_raw SMALLINT, soil_pct NUMERIC(5,2),
    dht_valid BOOLEAN DEFAULT true, soil_valid BOOLEAN DEFAULT true,
    wifi_rssi SMALLINT
);
SELECT create_hypertable('sensor_readings','time',chunk_time_interval=>INTERVAL '1 day');
CREATE INDEX ON sensor_readings (device_id, time DESC);
ALTER TABLE sensor_readings SET (timescaledb.compress,
    timescaledb.compress_orderby='time DESC', timescaledb.compress_segmentby='device_id');
SELECT add_compression_policy('sensor_readings', INTERVAL '7 days');
SELECT add_retention_policy('sensor_readings', INTERVAL '1 year');

-- ── Continuous Aggregates ─────────────────────────────────
CREATE MATERIALIZED VIEW sensor_5min
WITH (timescaledb.continuous) AS
SELECT time_bucket('5 minutes',time) AS bucket, device_id,
    ROUND(AVG(temperature)::numeric,2) AS temp_avg,
    ROUND(MIN(temperature)::numeric,2) AS temp_min,
    ROUND(MAX(temperature)::numeric,2) AS temp_max,
    ROUND(AVG(humidity)::numeric,2) AS hum_avg,
    ROUND(AVG(soil_pct)::numeric,2) AS soil_avg, COUNT(*) AS readings
FROM sensor_readings WHERE dht_valid=true GROUP BY bucket,device_id WITH NO DATA;
SELECT add_continuous_aggregate_policy('sensor_5min',
    start_offset=>INTERVAL '1 hour', end_offset=>INTERVAL '5 minutes',
    schedule_interval=>INTERVAL '5 minutes');

CREATE MATERIALIZED VIEW sensor_1h
WITH (timescaledb.continuous) AS
SELECT time_bucket('1 hour',time) AS bucket, device_id,
    ROUND(AVG(temperature)::numeric,2) AS temp_avg,
    ROUND(MIN(temperature)::numeric,2) AS temp_min,
    ROUND(MAX(temperature)::numeric,2) AS temp_max,
    ROUND(AVG(humidity)::numeric,2) AS hum_avg,
    ROUND(AVG(soil_pct)::numeric,2) AS soil_avg, COUNT(*) AS readings
FROM sensor_readings WHERE dht_valid=true GROUP BY bucket,device_id WITH NO DATA;
SELECT add_continuous_aggregate_policy('sensor_1h',
    start_offset=>INTERVAL '2 hours', end_offset=>INTERVAL '1 hour',
    schedule_interval=>INTERVAL '1 hour');

CREATE MATERIALIZED VIEW sensor_1d
WITH (timescaledb.continuous) AS
SELECT time_bucket('1 day',time) AS bucket, device_id,
    ROUND(AVG(temperature)::numeric,2) AS temp_avg,
    ROUND(MIN(temperature)::numeric,2) AS temp_min,
    ROUND(MAX(temperature)::numeric,2) AS temp_max,
    ROUND(AVG(humidity)::numeric,2) AS hum_avg,
    ROUND(AVG(soil_pct)::numeric,2) AS soil_avg, COUNT(*) AS readings
FROM sensor_readings WHERE dht_valid=true GROUP BY bucket,device_id WITH NO DATA;
SELECT add_continuous_aggregate_policy('sensor_1d',
    start_offset=>INTERVAL '3 days', end_offset=>INTERVAL '1 day',
    schedule_interval=>INTERVAL '1 day');

-- ── Restul tabelelor ──────────────────────────────────────
CREATE TABLE actuator_events (
    time TIMESTAMPTZ NOT NULL DEFAULT NOW(), device_id VARCHAR(50) NOT NULL,
    device_name VARCHAR(50) NOT NULL, action VARCHAR(20) NOT NULL,
    triggered_by VARCHAR(50) DEFAULT 'auto', duration_ms INT,
    user_id INT REFERENCES users(id) ON DELETE SET NULL, notes TEXT
);
SELECT create_hypertable('actuator_events','time');
CREATE INDEX ON actuator_events (device_id, time DESC);

CREATE TABLE irrigation_schedules (
    id SERIAL PRIMARY KEY, device_id VARCHAR(50) NOT NULL,
    name VARCHAR(100) NOT NULL, cron_expr VARCHAR(100) NOT NULL,
    duration_s INT NOT NULL DEFAULT 60, enabled BOOLEAN DEFAULT true,
    last_run TIMESTAMPTZ, run_count INT DEFAULT 0,
    created_by INT REFERENCES users(id) ON DELETE SET NULL,
    created_at TIMESTAMPTZ DEFAULT NOW(), updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE irrigation_log (
    time TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    schedule_id INT REFERENCES irrigation_schedules(id) ON DELETE SET NULL,
    device_id VARCHAR(50) NOT NULL, duration_s INT,
    triggered_by VARCHAR(50) DEFAULT 'schedule',
    success BOOLEAN DEFAULT true, notes TEXT
);
SELECT create_hypertable('irrigation_log','time');

CREATE TABLE alert_rules (
    id SERIAL PRIMARY KEY, device_id VARCHAR(50) NOT NULL,
    name VARCHAR(100) NOT NULL, parameter VARCHAR(50) NOT NULL,
    operator VARCHAR(3) NOT NULL CHECK (operator IN ('>','<','>=','<=','=')),
    threshold NUMERIC(8,2) NOT NULL, enabled BOOLEAN DEFAULT true,
    notify_email BOOLEAN DEFAULT true, notify_sms BOOLEAN DEFAULT false,
    cooldown_min INT DEFAULT 30,
    severity VARCHAR(10) DEFAULT 'warning' CHECK (severity IN ('info','warning','critical')),
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE alert_history (
    time TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    rule_id INT REFERENCES alert_rules(id) ON DELETE SET NULL,
    device_id VARCHAR(50), parameter VARCHAR(50),
    value NUMERIC(8,2), threshold NUMERIC(8,2), message TEXT,
    severity VARCHAR(10) DEFAULT 'warning',
    email_sent BOOLEAN DEFAULT false, sms_sent BOOLEAN DEFAULT false,
    acknowledged BOOLEAN DEFAULT false,
    ack_by INT REFERENCES users(id) ON DELETE SET NULL, ack_at TIMESTAMPTZ
);
SELECT create_hypertable('alert_history','time');
CREATE INDEX ON alert_history (device_id, time DESC);
CREATE INDEX ON alert_history (acknowledged) WHERE NOT acknowledged;

CREATE TABLE refresh_tokens (
    id BIGSERIAL PRIMARY KEY, user_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash VARCHAR(255) NOT NULL UNIQUE, expires_at TIMESTAMPTZ NOT NULL,
    revoked BOOLEAN DEFAULT false, created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE settings (key VARCHAR(100) PRIMARY KEY, value TEXT, updated_at TIMESTAMPTZ DEFAULT NOW());

-- Date initiale
INSERT INTO devices VALUES ('SERA_001','Sera Principala','Gradina',NULL,NULL,false,NOW());
-- Parola: Admin1234! — SCHIMBA IMEDIAT!
INSERT INTO users (username,email,password,role) VALUES
    ('admin','admin@sera.local','$2b$12$LQv3c1yqBWVHxkd0LHAkCOYz6TiYnMl9v.yKQCpvXDwxkFgO3MK1i','admin');
INSERT INTO alert_rules (device_id,name,parameter,operator,threshold,severity) VALUES
    ('SERA_001','T critica mare','temperature','>',35,'critical'),
    ('SERA_001','T critica mica','temperature','<',5,'critical'),
    ('SERA_001','Umiditate mare','humidity','>',90,'warning'),
    ('SERA_001','Sol uscat','soil_pct','<',20,'warning');
INSERT INTO settings VALUES ('data_retention_days','365',NOW()),('timezone','Europe/Bucharest',NOW());
