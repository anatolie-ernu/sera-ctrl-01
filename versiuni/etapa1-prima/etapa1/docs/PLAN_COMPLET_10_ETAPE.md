# SERA INTELIGENTĂ — PLAN COMPLET 10 ETAPE

Acesta este documentul master cu planul tuturor etapelor proiectului.

---

## Prezentare Generală

| Etapă | Titlu | Componente | Status |
|-------|-------|-----------|--------|
| 1 | **Firmware ESP32 - Hardware de bază** | Senzori, Relee, Logică locală | ✅ Livrat |
| 2 | **ESP32 - WiFi + MQTT** | WiFi Manager, MQTT Client, NTP | 🔜 Următor |
| 3 | **Backend Node.js - Nucleu** | Express API, MQTT Broker, TimescaleDB | 🔜 |
| 4 | **Backend - Scheduler Irigare** | node-cron, Calendar, Programe | 🔜 |
| 5 | **Backend - Alerte și Notificări** | Email, SMS, Reguli configurabile | 🔜 |
| 6 | **Docker + CI/CD GitLab** | Docker Compose, Nginx, SSL, Pipeline | 🔜 |
| 7 | **Frontend Web Dashboard** | React.js, Grafice, Control manual | 🔜 |
| 8 | **App Android (React Native)** | Dashboard mobil, Control, Push notif. | 🔜 |
| 9 | **Securitate și Producție** | JWT, HTTPS, Rate limiting, Backup | 🔜 |
| 10 | **Integrare Finală + Testare** | E2E testing, Monitoring, Documentație | 🔜 |

---

## Etapa 1: Firmware ESP32 - Hardware de bază ✅

**Obiectiv:** Sera funcționează autonom, fără internet.

**Fișiere livrate:**
```
firmware/
├── src/
│   ├── main.cpp
│   ├── config.h
│   ├── sensors.h / sensors.cpp
│   ├── actuators.h / actuators.cpp
│   ├── wifi_manager.h (placeholder)
│   └── display.h (placeholder)
├── platformio.ini
└── schema_conexiuni.txt
docs/
└── DOCUMENTATIE_ETAPA1.md
```

**Hardware acoperit:**
- ESP32 DevKit (WiFi integrat, dual-core 240MHz)
- DHT22 — temperatură și umiditate aer
- Senzor capacitiv sol
- Modul releu 5 canale:
  - 2× motoare ferestre (releu 1 și 2)
  - 1× robinet electric 220V (releu 3)
  - 2× ventilatoare 220V (releu 4 și 5)

**Funcționalități:**
- Citire senzori la 30 secunde
- Logică automată: deschide/închide ferestre după temperatură
- Logică automată: pornește/oprește ventilatoare după temperatură
- Alerte seriale la depășiri critice
- Interfață testare prin Serial Monitor (12 comenzi)
- Debounce relee (evită comutări rapide)
- Oprire de urgență

---

## Etapa 2: ESP32 - WiFi + MQTT (urmează)

**Obiectiv:** ESP32 trimite date la server și primește comenzi.

**Ce se adaugă:**
- WiFiManager cu portal captiv (configurare SSID/parolă fără recompilare)
- MQTT Client (PubSubClient) — publish senzori, subscribe comenzi
- Topics MQTT:
  - `greenhouse/sensors` → publică date JSON la 30s
  - `greenhouse/cmd/windows` → primește ON/OFF
  - `greenhouse/cmd/pump` → primește ON/OFF/duration
  - `greenhouse/cmd/fan` → primește ON/OFF
  - `greenhouse/status` → heartbeat la 60s
  - `greenhouse/alerts` → publică alerte critice
- NTP time sync (ora exactă pentru loguri)
- Reconnect automat WiFi și MQTT
- Persistare stare în NVS (EEPROM) la resetare

---

## Etapa 3: Backend Node.js - Nucleu

**Obiectiv:** Server care primește și stochează date, expune API REST.

**Ce se construiește:**
- Node.js + Express.js — API REST
- Mosquitto MQTT Broker în Docker
- TimescaleDB (PostgreSQL) — stocare time-series
- Tabel sensor_readings, actuator_events
- Endpoint-uri API:
  - `GET /api/sensors/latest` — ultimele valori
  - `GET /api/sensors/history?from=&to=&interval=` — istoric
  - `POST /api/actuators/command` — trimite comandă la ESP32
  - `GET /api/actuators/events` — istoric acțiuni
- Autentificare JWT (baza)
- Logging structurat (Winston)

---

## Etapa 4: Backend - Scheduler Irigare

**Obiectiv:** Programare automată a udatului după calendar.

**Ce se construiește:**
- Tabel `irrigation_schedules` în DB
- API CRUD pentru programe:
  - Frecvență: de X ori/zi, zile specifice din săptămână
  - Cantitate: după timp (secunde) sau volum (dacă ai debitmetru)
  - Activat/dezactivat per program
- node-cron — execuție automată
- Logare fiecare ciclu de udare
- UI pentru definire programe (în Etapa 7)

**Exemplu program:**
```json
{
  "name": "Udare dimineata",
  "cron": "0 7 * * 1,3,5",
  "duration_seconds": 120,
  "enabled": true
}
```

---

## Etapa 5: Backend - Alerte și Notificări

**Obiectiv:** Notificări email/SMS la incidente și devieri.

**Ce se construiește:**
- Tabel `alert_rules` — reguli configurabile din UI
- Motor de evaluare alerte (rulat la fiecare citire senzori)
- Nodemailer — trimitere email (SMTP Gmail/SendGrid)
- Twilio sau SMS Gateway — trimitere SMS
- Cooldown per alertă (evită spam — max 1 SMS/30 min per tip alertă)
- Tabel `alert_history` — istoricul tuturor alertelor trimise
- Tipuri de alerte:
  - Temperatură > prag (configurabil)
  - Temperatură < prag (configurabil)
  - Umiditate > prag
  - Umiditate sol < prag (sol uscat)
  - Dispozitiv offline (heartbeat lipsă)

---

## Etapa 6: Docker + CI/CD GitLab

**Obiectiv:** Deployment automat, producție stabilă.

**Ce se construiește:**
- `docker-compose.yml` — development
- `docker-compose.prod.yml` — producție cu volume persistente
- Dockerfile optimizat pentru backend (multi-stage build)
- Nginx — reverse proxy cu SSL/TLS
- Let's Encrypt (Certbot) — certificate SSL gratuite
- `.gitlab-ci.yml` cu pipeline:
  - Stage `test` — rulează unit tests
  - Stage `build` — build Docker image
  - Stage `deploy` — SSH deploy pe server Linux
- Variabile CI/CD securizate (secrets în GitLab)
- Health checks pentru toate serviciile

---

## Etapa 7: Frontend Web Dashboard

**Obiectiv:** Interfață web completă pentru management seră.

**Ce se construiește:**
- React.js + Tailwind CSS
- Dashboard principal:
  - Valori live senzori (refresh 30s)
  - Stare dispozitive cu toggle manual
  - Grafic temperatură ultimele 24h
- Pagina Grafice:
  - Temperatură, umiditate, sol
  - Intervale: 24h, 7 zile, 30 zile, custom
  - Export CSV
- Pagina Scheduler — calendar vizual irigare
- Pagina Alerte — configurare reguli + istoric
- Pagina Loguri — toate evenimentele filtrate
- Pagina Setări — praguri, dispozitive, cont
- Autentificare completă (login, JWT refresh)
- Grafana embed opțional pentru grafice avansate

---

## Etapa 8: App Android (React Native)

**Obiectiv:** Control complet de pe telefon.

**Ce se construiește:**
- React Native + Expo sau bare workflow
- Toate ecranele din versiunea web adaptate mobil:
  - Dashboard cu date live
  - Control manual dispozitive
  - Grafice interactive
  - Programare irigare
  - Setări alerte
- Notificări Push (Firebase FCM)
- Funcționare offline pentru vizualizare date cache
- Fingerprint/FaceID pentru securitate
- Widget Android (opțional) — temperatură pe ecran principal

---

## Etapa 9: Securitate și Producție

**Obiectiv:** Sistem securizat, stabil, pregătit pentru utilizare îndelungată.

**Ce se implementează:**
- HTTPS peste tot (API + MQTT over TLS)
- JWT cu refresh tokens și blacklist
- Rate limiting API (express-rate-limit)
- Sanitizare input-uri (validare Joi/Zod)
- MQTT authentication (username/password)
- Backup automat bază de date (cron zilnic → S3 sau local)
- Monitoring server (Uptime Robot sau self-hosted Uptime Kuma)
- Log rotation (evitare umplere disk)
- Documentație API (Swagger/OpenAPI)
- Variabile de mediu securizate (nu hardcoded)

---

## Etapa 10: Integrare Finală + Testare

**Obiectiv:** Sistem complet, testat, documentat.

**Ce se face:**
- Testare end-to-end (E2E) cu scenarii reale:
  - Simulare temperatură ridicată → ferestre deschide + email alertă
  - Scheduler irigare → pompă pornește + loghează
  - Pierdere WiFi ESP32 → alertă offline → reconectare automată
- Load testing API (Artillery.io)
- Documentație completă utilizator (manual PDF)
- Diagrama arhitectură finală actualizată
- Checklist pre-instalare în seră reală
- Proceduri de backup și recovery
- Ghid troubleshooting complet

---

*Plan generat pentru: Sera Inteligentă IoT Project v1.0*
*Total etape: 10 | Etapă curentă: 1*
