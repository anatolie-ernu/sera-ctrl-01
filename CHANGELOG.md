# Changelog — SERA-CTRL-01

## [3.0.0] — 2026-06
### Added
- Firmware producție: NVS, portal captiv SoftAP, OTA HTTPS cu SHA-256
- Secure Boot v2 + anti-rollback OTA
- Factory test automat EOL (< 90s/unitate)
- PKI: Root CA + Device CA + certificate X.509 per dispozitiv
- MQTTS (port 8883) cu autentificare prin certificat
- Backend multi-tenant cu Row-Level Security PostgreSQL
- Înrolare dispozitiv prin cod QR (`/api/claim`)
- Fleet management OTA cu rollout eșalonat (10%/oră)
- Aplicație mobilă React Native (Android + iOS)
- Monitoring Prometheus + Grafana (Fleet Overview dashboard)
- Schema electrică SERA-CTRL-01 (SVG vectorial + PDF A2)

## [2.0.0] — 2026-03
### Added
- WiFi + MQTT (etapa2 firmware)
- Backend Node.js + TimescaleDB + Redis
- Docker Compose stack complet
- Alerte email + SMS
- Scheduler irigare cu programe recurente

## [1.0.0] — 2026-01
### Added
- Firmware standalone ESP32 (etapa1)
- Control relee pe praguri locale
- Senzori SHT31 + sol capacitiv
