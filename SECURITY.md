# Politică de securitate — SERA-CTRL-01

## Raportarea vulnerabilităților

Dacă descoperiți o vulnerabilitate de securitate în acest proiect,
vă rugăm să nu o publicați public. Contactați echipa la:

**Email:** security@[domeniu-organizatie]

Vom răspunde în maxim 72 de ore și vom colabora pentru remedierea problemei.

## Versiuni suportate

| Versiune firmware | Suport securitate |
|-------------------|-------------------|
| 3.x (producție)   | ✅ Da             |
| 2.x (etapa2)      | ⚠️ Doar critice   |
| 1.x (etapa1)      | ❌ Nu             |

## Practici de securitate implementate

- **Secure Boot v2** pe ESP32 — firmware semnat criptografic
- **OTA HTTPS** cu verificare SHA-256 și anti-rollback
- **Certificate X.509** per dispozitiv pentru MQTTS (port 8883)
- **JWT** cu expirare scurtă (15 min) + refresh token (30 zile)
- **Row-Level Security** PostgreSQL pentru izolarea tenantilor
- **Sloturi PCB** pentru creepage 6.4mm (conformitate LVD)

## Ce NU se commit-uiește niciodată

- Chei private CA (`.key`, `.pem`)
- Certificate device produse
- Fișiere `.env` cu secrete reale
- Parole baze de date de producție
