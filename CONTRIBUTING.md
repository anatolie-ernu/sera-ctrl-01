# Ghid de contribuție — SERA-CTRL-01

## Fluxul de lucru

1. Fork → branch din `develop` (nu din `main`)
2. Denumire branch: `feature/descriere`, `fix/descriere`, `docs/descriere`
3. Commit-uri mici și clare: `feat: adaugă suport DHT22`, `fix: corectare timeout MQTT`
4. Pull Request → `develop` cu descriere completă
5. Review obligatoriu înainte de merge

## Structura proiectului

```
firmware/        — Cod C++ PlatformIO (ESP32)
backend/         — Node.js + Express + TimescaleDB
mobile/          — React Native (Expo) — Android + iOS
infrastructure/  — Docker Compose, Mosquitto, Nginx, Monitoring
production/      — PKI (CA), jig de testare linie producție
docs/            — Documentație PDF, scheme electrice
scripts/         — Utilitare server Linux
tests/           — Teste E2E
```

## Convenții cod

- **Firmware:** C++17, comentarii în engleză, indentare 2 spații
- **Backend:** Node.js 20+, ESLint Airbnb, async/await (nu callbacks)
- **Mobile:** React Native + Expo, functional components + hooks
- **Commits:** Conventional Commits (feat/fix/docs/chore/refactor)

## Testare locală

```bash
# Backend
cd backend && npm install && npm test

# Firmware
cd firmware/etapa1 && pio run --target upload

# Mobile
cd mobile && npm install && npx expo start
```
