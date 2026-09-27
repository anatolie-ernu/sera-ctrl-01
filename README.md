# 🌿 SERA-CTRL-01 — Controler Automatizare Seră Inteligentă

[![CI](https://github.com/[org]/sera-ctrl-01/actions/workflows/ci.yml/badge.svg)](https://github.com/[org]/sera-ctrl-01/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](firmware/)
[![Node.js](https://img.shields.io/badge/Node.js-20+-brightgreen)](backend/)

Sistem IoT complet pentru automatizarea serei — de la firmware ESP32 la backend SaaS multi-tenant, aplicație mobilă și linie de producție industrializată.

---

## 📐 Schema electrică

| Format | Fișier | Descriere |
|--------|--------|-----------|
| SVG (vectorial) | [`docs/schemas/sera_ctrl_01_schema.svg`](docs/schemas/sera_ctrl_01_schema.svg) | Scalabil, editabil |
| PDF (tipărit) | [`docs/schemas/SERA-CTRL-01_Schema_Electrica.pdf`](docs/schemas/SERA-CTRL-01_Schema_Electrica.pdf) | A2 landscape |
| PNG (4800px) | [`docs/schemas/SERA-CTRL-01_Schema_Electrica_HiRes.png`](docs/schemas/SERA-CTRL-01_Schema_Electrica_HiRes.png) | Previzualizare |

> ⚡ **Atenție:** Schema conține circuite de 230V AC. Instalarea se face exclusiv de electrician autorizat.

---

## 🏗️ Arhitectură sistem

```
┌─────────────────────────────────────────────────────────────────┐
│                        Cloud / Server Linux                      │
│  ┌──────────────┐  ┌──────────┐  ┌─────────┐  ┌─────────────┐ │
│  │  Node.js API │  │Mosquitto │  │TimescaleDB  │  Redis      │ │
│  │  (Express)   │◄─│  MQTTS   │  │(PostgreSQL)│  (cache)    │ │
│  └──────┬───────┘  └────▲─────┘  └─────────┘  └─────────────┘ │
│         │               │              ▲                         │
└─────────┼───────────────┼──────────────┼─────────────────────────┘
          │ REST/HTTPS    │ MQTT TLS     │ SQL
    ┌─────▼──────┐   ┌───┴──────────────┴───┐
    │   App      │   │    ESP32-WROOM-32E    │
    │  Mobilă    │   │    SERA-CTRL-01       │
    │ (React     │   │  ┌──────┐ ┌────────┐ │
    │  Native)   │   │  │SHT31 │ │Senzor  │ │
    └────────────┘   │  │T/RH  │ │Sol ADC │ │
                     │  └──────┘ └────────┘ │
                     │  ┌─────────────────┐  │
                     │  │ 5× Relee HF115F │  │
                     │  │ K1 Fereastră 1  │  │
                     │  │ K2 Fereastră 2  │  │
                     │  │ K3 Electrovalvă │  │
                     │  │ K4 Ventilator 1 │  │
                     │  │ K5 Ventilator 2 │  │
                     │  └─────────────────┘  │
                     └───────────────────────┘
```

---

## 📁 Structura repository

```
sera-ctrl-01/
├── firmware/
│   ├── etapa1/          # ESP32 standalone (fără WiFi, testare locală)
│   ├── etapa2/          # ESP32 + WiFi/MQTT
│   └── production/      # Firmware producție: NVS, OTA, Secure Boot
├── backend/
│   ├── src/             # Node.js Express API
│   │   ├── api/         # Routes + middleware (auth, tenant)
│   │   ├── mqtt/        # Client MQTT + procesare mesaje
│   │   ├── alerts/      # Motor alerte (reguli + notificări)
│   │   ├── scheduler/   # Programe irigare automate
│   │   └── notifications/ # Email (Nodemailer) + SMS (Twilio)
│   └── db/init/         # Schema TimescaleDB + migrări multi-tenant
├── mobile/              # React Native (Expo) — Android + iOS
│   └── src/
│       ├── screens/     # Login, Claim QR, Dashboard
│       ├── services/    # API client + Push notifications
│       └── navigation/  # Stack navigator + auth flow
├── infrastructure/
│   ├── mosquitto/       # Config MQTT + TLS + ACL
│   ├── nginx/           # Reverse proxy + SSL
│   └── monitoring/      # Prometheus + Grafana fleet dashboard
├── production/
│   ├── ca/              # PKI: Root CA + Device CA + cert/dispozitiv
│   └── jig/             # Stație test EOL automatizată (Python)
├── docs/
│   ├── pdf/             # Documentație industrializare P0–P6 + manuale
│   └── schemas/         # Schemă electrică (SVG + PDF + PNG)
├── scripts/             # Utilitare server: backup, health-check, SSL
├── tests/               # Teste E2E (Jest)
└── docker-compose.yml   # Stack complet: DB, MQTT, Redis, API, Nginx
```

---

## 🚀 Pornire rapidă (development)

### Cerințe
- Docker + Docker Compose v2
- Node.js 20+
- Python 3.11+ (pentru jig testare)
- PlatformIO (pentru firmware)

### 1. Backend + infrastructură
```bash
git clone https://github.com/[org]/sera-ctrl-01.git
cd sera-ctrl-01

# Copiați și completați variabilele de mediu
cp .env.example .env
# Editați .env cu parolele reale

# Porniți stack-ul complet
docker compose up -d

# Verificați că toate serviciile sunt UP
docker compose ps
```

### 2. Firmware (PlatformIO)
```bash
cd firmware/etapa2
pio run --target upload --upload-port /dev/ttyUSB0
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

### 3. Aplicație mobilă
```bash
cd mobile
npm install
npx expo start
# Scanați QR-ul cu Expo Go (Android/iOS)
```

### 4. Monitoring (opțional)
```bash
docker compose -f docker-compose.yml \
               -f infrastructure/docker-compose.monitoring.yml up -d
# Grafana: http://localhost:3001
```

---

## 🔌 Hardware SERA-CTRL-01

| Componentă | Referință | Rol |
|------------|-----------|-----|
| MCU | ESP32-WROOM-32E-N8 | Controller principal, WiFi, OTA |
| Sursă AC-DC | HLK-10M05 | 230V→5V izolat, 2A |
| Regulator LDO | ME6217 | 5V→3.3V, 800mA |
| Driver relee | ULN2003A | Darlington 7-canal, flyback intern |
| Relee | HF115F-005-1ZS3 | 5×, 12A/250V~, pin C/NO/NC |
| Senzor T/RH | SHT31 | I2C, ±0.2°C, ±2% RH |
| Senzor sol | Capacitiv | ADC GPIO34, 0–100% |
| Transciver RS485 | MAX3485 | Modbus extern (opțional) |
| Protecție | MOV 470V | Supratensiune linie 230V |
| Siguranță | 2A T (lentă) | Protecție linie L |

**GPIO map:** vezi [`firmware/production/src/hardware.h`](firmware/production/src/hardware.h)

---

## 📋 Documentație

| Document | Descriere |
|----------|-----------|
| [P0 — Plan Master](docs/pdf/P0_Plan_Master_Industrializare.pdf) | Faze, buget (30–60k€), echipă, riscuri |
| [P1 — Hardware](docs/pdf/P1_Proiectare_Hardware.pdf) | Schemă PCB, BOM, carcasă |
| [P2 — Prototip](docs/pdf/P2_Prototip_si_Validare.pdf) | Bring-up, validare, plan teste |
| [P3 — Firmware](docs/pdf/P3_Firmware_de_Productie.pdf) | NVS, OTA, Secure Boot, factory test |
| [P4 — Certificare CE](docs/pdf/P4_Certificare_CE.pdf) | LVD/EMC/RED/RoHS, dosar tehnic |
| [P5 — Backend SaaS](docs/pdf/P5_Backend_SaaS_si_Aplicatie.pdf) | Multi-tenant, MQTTS, fleet OTA |
| [P6 — Producție](docs/pdf/P6_Productie_in_Serie.pdf) | EMS, DFM, jig, cost unitar |
| [Manual utilizare](docs/pdf/Manual_Utilizare_SERA-CTRL-01.pdf) | Instalare, configurare, troubleshooting |
| [Declarație Conformitate UE](docs/pdf/Declaratie_Conformitate_UE.pdf) | Template DoC — de completat |
| [Ghid master lansare](docs/pdf/Ghid_Master_Lansare.pdf) | Sinteza + checklist 16 pași |

---

## 🔐 Securitate

- Secure Boot v2 pe ESP32 (firmware semnat)
- OTA prin HTTPS cu SHA-256 și anti-rollback
- Certificate X.509 per dispozitiv pentru MQTTS
- JWT 15min + refresh 30 zile
- Row-Level Security PostgreSQL (izolare tenanți)
- Creepage PCB 6.4mm (conformitate LVD 2014/35/UE)

Vezi [SECURITY.md](SECURITY.md) pentru raportarea vulnerabilităților.

---

## 📄 Licență

MIT © 2026 — SERA-CTRL-01 Project

> Componentele hardware (schemă electrică, BOM) sunt licențiate separat sub CERN-OHL-S v2.

---

## 🤝 Contribuție

Citiți [CONTRIBUTING.md](CONTRIBUTING.md) pentru fluxul de lucru și convențiile de cod.
