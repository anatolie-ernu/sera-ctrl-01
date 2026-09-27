# Pasul 2 — Linia de producție + Backend SaaS v2

## production_line/ — stația de test End-Of-Line
| Fișier | Rol |
|--------|-----|
| ca_setup.sh | Creează CA-ul firmei (Root offline + Device CA) — RULAT O DATĂ |
| gen_device_cert.sh | Emite certificat X.509 unic per unitate (CN = device_id) |
| jig_test.py | Stația completă: flash → factory test → certificat → etichetă QR → baza de producție |

Flux operator: așază placa → ENTER → < 90 s → eticheta iese din imprimantă.

## backend_v2/ — migrarea SaaS multi-client
| Fișier | Rol |
|--------|-----|
| migrations/002_multitenant.sql | organizations + org_id + Row-Level Security + tabele OTA flotă |
| src/api/middleware/tenant.js | SET LOCAL app.org_id per request → RLS izolează clienții la nivel de DB |
| src/api/routes/claim.js | Înrolare dispozitiv prin QR (claim_code de pe etichetă) |
| src/api/routes/fleet.js | Versiuni firmware, promovare beta→stable, rollout eșalonat 10%/oră |
| mosquitto/mosquitto_tls.conf + acl.conf | MQTTS 8883, identitate = CN certificat, ACL per dispozitiv |

## Integrare în app.js existent
```js
const { tenant } = require('./api/middleware/tenant');
app.use('/api/claim', require('./api/routes/claim'));
app.use('/api/fleet', require('./api/routes/fleet').router);
// + cron orar pentru rollout:
cron.schedule('0 * * * *', require('./api/routes/fleet').processOtaQueue);
```
