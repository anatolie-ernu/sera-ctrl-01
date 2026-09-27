# 🌿 Greenhouse IoT System

**Full-stack greenhouse automation: ESP32 → MQTT → Node.js → TimescaleDB → Docker → GitLab CI/CD**

---

## Quick Start

### Firmware (ESP32)
```bash
cd firmware/etapa2/scripts
sudo ./setup.sh           # Install PlatformIO, configure udev rules
./set_wifi.sh             # Configure WiFi + MQTT credentials interactively
./calibrate_soil.sh       # Calibrate the soil moisture sensor
./flash.sh all            # Compile, upload, open Serial Monitor
```

### Server (Linux — Ubuntu 22.04+)
```bash
cp .env.example .env && nano .env    # Fill in all secrets
sudo ./scripts/install_server.sh     # Install Docker, UFW, systemd service
./scripts/start.sh                   # Start all 6 Docker containers
./scripts/ssl.sh                     # Issue Let's Encrypt certificate
./scripts/security_harden.sh         # Apply SSH hardening + fail2ban
```

**Default login:** admin / Admin1234! — **change immediately after first login**

---

## Architecture

```
ESP32 (C++ / PlatformIO)
  └─ MQTT → Mosquitto broker
                └─ Node.js backend
                      ├─ TimescaleDB (PostgreSQL + time-series)
                      ├─ Redis (cache / rate-limit)
                      └─ Nginx (SSL reverse proxy)

GitLab CI/CD: push to main → test → build Docker → SSH deploy
```

---

## Project Layout

| Path | Contents |
|------|----------|
| `firmware/etapa1/` | Autonomous firmware: sensors + relays, no WiFi |
| `firmware/etapa2/` | Adds WiFi, MQTT, NTP |
| `backend/`         | Node.js API server |
| `backend/db/init/` | TimescaleDB schema (auto-run on first Docker start) |
| `docker/`          | Mosquitto and Nginx configs |
| `docker-compose.yml` | All 6 services |
| `scripts/`         | One-command automation scripts |
| `tests/`           | Jest end-to-end test suite |
| `.gitlab-ci.yml`   | CI/CD pipeline |
| `docs/`            | Generated project description (DOCX) |

---

## Documentation

See `docs/Greenhouse_IoT_Project_Description.docx` for the full 13-chapter
technical document covering architecture, database design, API reference,
firmware details, and setup guides.
