# Pasul 3 — Aplicatia mobila comerciala + Monitoring flota

## mobile/ — React Native (Expo), Android + iOS din acelasi cod
| Fisier | Rol |
|--------|-----|
| App.js | Punct de intrare |
| src/navigation/AppNavigator.js | Restaurare sesiune la pornire → Login / Claim / Dashboard |
| src/screens/LoginScreen.js | Autentificare |
| src/screens/ClaimScreen.js | Onboarding: scanare QR → /api/claim → ghid WiFi |
| src/screens/DashboardScreen.js | Stare live, control manual, statistici 24h, alerte |
| src/services/api.js | Client HTTP cu refresh automat token + SecureStore |
| src/services/push.js | Notificari push FCM/Expo pentru alerte |

Build productie:  `eas build -p android --profile production`
Distributie: Google Play (cont unic 25$) + App Store (99$/an). Necesita politica GDPR.

## monitoring/ — Prometheus + Grafana
| Fisier | Rol |
|--------|-----|
| metrics.js | Endpoint /metrics in backend (devices, temp, OTA, alerte, MQTT) |
| prometheus/prometheus.yml | Scrape la 15s |
| prometheus/alert_rules.yml | Alerte infra: backend jos, >30% offline, esecuri OTA |
| grafana/dashboards/fleet.json | Tablou: online, flota, alerte, temp/device, stari OTA |
| docker-compose.monitoring.yml | Adauga Prometheus + Grafana la stack |

Pornire:  docker compose -f docker-compose.yml -f monitoring/docker-compose.monitoring.yml up -d
Acces Grafana: tunel SSH pe 3001 (nu expus public).

## Integrare in app.js
```js
const metrics = require('../monitoring/metrics');
app.use('/metrics', metrics);
// in mqtt/service.js: metrics.incMqtt() la fiecare mesaj
// in alerts/engine.js: metrics.incAlert(rule.severity) cand se declanseaza
```
