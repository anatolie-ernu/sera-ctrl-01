# Backend SERA-CTRL-01

Node.js + Express + TimescaleDB + MQTT + Redis

## Pornire locală
```bash
npm install
cp ../.env.example ../.env  # completați variabilele
docker compose up -d timescaledb mosquitto redis
npm run dev
```

## Structura API
- `POST /api/auth/login` — autentificare JWT
- `GET  /api/dashboard` — date live dispozitiv
- `POST /api/actuators/command` — comandă releu
- `GET  /api/sensors/history` — istoric senzori
- `POST /api/schedules` — program irigare
- `POST /api/claim` — înrolare dispozitiv nou (QR)
- `GET  /api/fleet` — management OTA flotă (admin)

## Baza de date
Schema TimescaleDB cu hypertables, continuous aggregates,
compression și data retention. Migrări în `db/init/`.
