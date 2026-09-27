-- =============================================================================
-- 002_multitenant.sql — Migrare la SaaS multi-client
-- =============================================================================
-- Rulare:  ./scripts/db_shell.sh < migrations/002_multitenant.sql
-- Idee centrală: fiecare rând aparține unei organizații (org_id).
-- Izolarea se aplică la nivel de BAZĂ DE DATE prin Row-Level Security (RLS):
-- chiar dacă un bug în API uită filtrul, PostgreSQL refuză rândurile străine.
-- =============================================================================
BEGIN;

-- ── 1. Organizații (clienții platformei) ─────────────────────────────────────
CREATE TABLE organizations (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(120) NOT NULL,
    plan        VARCHAR(20)  NOT NULL DEFAULT 'free'
                CHECK (plan IN ('free','pro','enterprise')),
    created_at  TIMESTAMPTZ DEFAULT NOW()
);
-- Organizația implicită preia datele existente (instalarea curentă)
INSERT INTO organizations (name, plan) VALUES ('Default', 'pro');

-- ── 2. org_id pe tabelele existente ──────────────────────────────────────────
ALTER TABLE users   ADD COLUMN org_id INT NOT NULL DEFAULT 1 REFERENCES organizations(id);
ALTER TABLE devices ADD COLUMN org_id INT          REFERENCES organizations(id);
-- Dispozitivele NEREVENDICATE au org_id NULL — apar doar prin claim
ALTER TABLE devices ADD COLUMN claim_code  VARCHAR(16);   -- codul de pe etichetă
ALTER TABLE devices ADD COLUMN claimed_at  TIMESTAMPTZ;
UPDATE devices SET org_id = 1;                            -- datele existente → Default

-- Hypertable-urile moștenesc izolarea prin device_id → devices.org_id;
-- pentru interogări directe adăugăm org_id și pe alert_rules/schedules:
ALTER TABLE alert_rules          ADD COLUMN org_id INT NOT NULL DEFAULT 1 REFERENCES organizations(id);
ALTER TABLE irrigation_schedules ADD COLUMN org_id INT NOT NULL DEFAULT 1 REFERENCES organizations(id);

-- ── 3. Row-Level Security ────────────────────────────────────────────────────
-- API-ul setează la fiecare request:  SET LOCAL app.org_id = '<org al userului>';
-- Politicile de mai jos fac restul — niciun rând străin nu trece.
CREATE OR REPLACE FUNCTION current_org_id() RETURNS INT AS
$$ SELECT NULLIF(current_setting('app.org_id', true), '')::INT $$
LANGUAGE sql STABLE;

ALTER TABLE devices              ENABLE ROW LEVEL SECURITY;
ALTER TABLE alert_rules          ENABLE ROW LEVEL SECURITY;
ALTER TABLE irrigation_schedules ENABLE ROW LEVEL SECURITY;
ALTER TABLE users                ENABLE ROW LEVEL SECURITY;

CREATE POLICY org_devices   ON devices              USING (org_id = current_org_id());
CREATE POLICY org_rules     ON alert_rules          USING (org_id = current_org_id());
CREATE POLICY org_schedules ON irrigation_schedules USING (org_id = current_org_id());
CREATE POLICY org_users     ON users                USING (org_id = current_org_id());

-- Serviciul MQTT (scrie citiri pentru TOATE dispozitivele) folosește un rol
-- separat care ocolește RLS — datele de senzori sunt filtrate prin JOIN devices.
CREATE ROLE sera_ingest NOINHERIT;
GRANT INSERT ON sensor_readings, actuator_events, alert_history TO sera_ingest;
ALTER TABLE devices FORCE ROW LEVEL SECURITY;   -- nici owner-ul nu ocolește

-- ── 4. Managementul flotei OTA ───────────────────────────────────────────────
CREATE TABLE firmware_releases (
    id          SERIAL PRIMARY KEY,
    version     VARCHAR(16) NOT NULL UNIQUE,     -- '3.1.0'
    url         TEXT NOT NULL,                   -- HTTPS către .bin semnat
    sha256      CHAR(64) NOT NULL,
    channel     VARCHAR(10) NOT NULL DEFAULT 'beta'
                CHECK (channel IN ('beta','stable')),
    released_at TIMESTAMPTZ DEFAULT NOW(),
    notes       TEXT
);

CREATE TABLE device_firmware_status (
    device_id     VARCHAR(50) PRIMARY KEY REFERENCES devices(device_id),
    current_ver   VARCHAR(16),
    target_ver    VARCHAR(16),
    ota_state     VARCHAR(16) DEFAULT 'idle'     -- idle|queued|sent|ok|failed
                  CHECK (ota_state IN ('idle','queued','sent','ok','failed')),
    updated_at    TIMESTAMPTZ DEFAULT NOW()
);

COMMIT;
