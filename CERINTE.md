# CERINȚE PROIECT — SERA-CTRL-01
## Sistem Automat de Gestionare Seră Inteligentă

> Document generat din istoricul complet al conversațiilor cu Claude AI.
> Acoperă toate sesiunile: mai 2026 – septembrie 2026.

---

## 1. CERINȚA INIȚIALĂ — Descrierea proiectului

> *„Am un proiect care va gestiona sera mea. Sera are 2 motoare comandate prin releu
> care deschide ferestrele, un robinet electric de 220V care deschide apa să ude,
> 2 ventilatoare comandate prin releu pentru ventilare, senzori de temperatură și
> umiditate, senzor de umiditate a solului, taskuri de udat, de deschis ferestre
> automat etc."*

**Hardware identificat:**
- 2× motoare 12V DC (5A fiecare) pentru ferestre — comandate prin relee
- 1× electrovalvă 12V DC pentru irigare automată
- 2× ventilatoare 12V DC pentru ventilare
- Senzor temperatură + umiditate aer (DHT22 → înlocuit cu SHT31)
- Senzor umiditate sol (capacitiv, ADC)
- Interfață RS485 (Modbus, senzori industriali opționali)
- MCU: ESP32-WROOM-32E-N8 (8MB, WiFi, BT)

---

## 2. PLAN ÎN ETAPE

> *„Vreau să faci plan în 10 etape, să le generezi pe rând și să mi le dai ca
> arhive cu denumire etapa 1 cod sursă și așa mai departe, cu documentarea
> detaliată a fiecărei etape."*

**Etapele planificate și livrate:**

| Etapă | Conținut | Arhivă |
|-------|----------|--------|
| Etapa 1 | Firmware ESP32 standalone (fără WiFi) — senzori + relee pe praguri locale | `Etapa1_Firmware_ESP32.zip` |
| Etapa 2 | Firmware ESP32 + WiFi/MQTT — conectare broker, LWT, NTP, reconnect | `Etapa2_Firmware_WiFi_MQTT.zip` |
| Etapa 3 | Backend Node.js + TimescaleDB + Docker — API, MQTT service, hypertables | `Etapa3_Backend_TimescaleDB.zip` |
| Etapa 4 | Scheduler irigare avansat — programe recurente, condiții multiple | *(inclus în Etapa 3)* |
| Etapa 5 | Motor alerte + notificări email/SMS (Nodemailer + Twilio) | *(inclus în Etapa 3)* |
| Etapa 6 | CI/CD GitLab — pipeline complet build/test/deploy | `Etapa6_GitLab_CICD.zip` |
| Etapa 7 | Frontend React *(planificat)* | — |
| Etapa 8 | Aplicație mobilă React Native (Expo) | *(inclus în Pas 3)* |
| Etapa 9 | Securitate — JWT, TLS, Secure Boot, audit | `Etapa9_Securitate.zip` |
| Etapa 10 | Teste E2E — Jest, scenarii complete | `Etapa10_Teste_E2E.zip` |

---

## 3. CERINȚĂ SCRIPTURI + BAZĂ DE DATE

> *„Vreau să faci scripturi pentru automatizare maximă, baza de date să fie MySQL."*
> *„Dacă consideri că TimescaleDB e mai bună, putem merge pe TimescaleDB."*

**Decizie adoptată:** TimescaleDB (PostgreSQL + extensie time-series)
- Interogări time-series de 10-100× mai rapide
- `time_bucket()` nativ pentru grafice
- Compresie automată date vechi
- Continuous aggregates pre-calculate
- Retention policies automate

**Scripturi generate:** backup, health-check, SSL, install-server, MQTT user management,
security hardening, start/stop stack.

---

## 4. CERINȚĂ PACHET COMPLET CU DOCUMENTAȚIE

> *„Based on all of this, give me a detailed description of the entire project,
> a folder and file structure for the entire project and then give me the zip
> that contains all the code with comments that explain the code in detail."*

**Livrat:** `Greenhouse_IoT_Complete.zip` — 61 fișiere, cod comentat complet,
structură de directoare documentată, README detaliat.

---

## 5. CERINȚĂ INDUSTRIALIZARE — PRODUS COMERCIAL

> *„Pot să îmbunătățesc proiectul până la nivelul unui proiect matur, astfel ca
> el să poată fi produs la uzină?"*
>
> *„Da, vreau să pornim transformarea într-un proiect fiabil, pas cu pas,
> cu schemă, plată pentru producție, prototip, aplicație și tot ce trebuie."*
>
> *„Vreau și documentație detaliată PDF pe fiecare etapă."*

**Pași de industrializare livrați:**

| Document | Conținut |
|----------|----------|
| P0 — Plan Master | Buget 30-60k€, timeline 9-12 luni, echipă, riscuri |
| P1 — Proiectare Hardware | PCB rev.B, BOM cu coduri LCSC, carcasă IP54 |
| P2 — Prototip și Validare | Bring-up, plan teste, criterii acceptanță |
| P3 — Firmware Producție | NVS, OTA A/B, Secure Boot v2, factory test |
| P4 — Certificare CE | LVD/EMC/RED/RoHS, dosar tehnic, creepage |
| P5 — Backend SaaS | Multi-tenant, MQTTS, fleet OTA, PKI |
| P6 — Producție Serie | Cost unitar 62-91€, preț vânzare 199-299€, EMS |

**Documente suplimentare:**
- Manual utilizare SERA-CTRL-01 (PDF)
- Declarație Conformitate UE (template DoC)
- Specificație etichetă produs
- Ghid master lansare (checklist 16 pași)

---

## 6. CERINȚĂ SCHEMĂ ELECTRICĂ — WIRING DIAGRAM

> *„Vreau o schemă logică grafică cu conexiuni."*
> *„Da, după o schemă electrică."*

**Schema E00 (wiring diagram general):**
- J1 terminal 2P 230V, F1 2A T, MOV RV1 470V
- PS1 HLK-10M05 (230V→5V izolat, cert. CE/UL)
- U2 ME6217 LDO (5V→3.3V, 800mA)
- ESP32-WROOM-32E-N8 cu GPIO map complet
- U4 ULN2003A (Darlington 7-canal, flyback intern)
- K1-K5 relee HF115F-005-1ZS3 cu pini C/NO/NC
- J3-J7 terminale bloc de ieșire
- Senzori: SHT31 (I2C), sol (ADC), RS485

**Fișiere:** `SERA-CTRL-01_Schema_Electrica.svg` / `.pdf` / `.png`

---

## 7. CERINȚĂ SCHEME CLARE — TEXT SEPARAT DE FIRE

> *„Cam s-au cățărat scrisul peste linii și nu prea se înțelege, poți să le
> faci mai clare și SVG tot în imagine să fie sau PDF?"*
>
> *„În schema electrică ai cățărat linii peste text și nu prea se înțelege."*

**Principii aplicate în redesenare:**
- Fire NUMAI în coridoare dedicate (orizontale/verticale stricte 90°)
- Text NICIODATĂ pe fire — etichete deasupra sau dedesubt componentelor
- Zone rectangulare fixe per bloc (AC, Solar, MPPT, Acumulator, Sarcini, Control)
- Pini C/NO/NC etichetați exterior blocului, în spațiu alb dedicat

---

## 8. CERINȚĂ IMAGINI STANDARD PNG

> *„Poți să-mi dai și o versiune în imagine standard a schemei SVG?"*

**Formate livrate:**
- PNG 1920px (Full HD — e-mail, documente Word)
- PNG 2560px (2K/QHD — prezentări, GitHub preview) ← **recomandat**
- PNG 4800px (Hi-Res pentru tipar)
- PDF vectorial (A2 landscape, scalabil infinit)
- SVG vectorial (editabil Inkscape/KiCad)

---

## 9. CERINȚĂ SCHEMĂ IEC INGINEREASCĂ + SISTEM 12V UPS SOLAR

> *„Vreau și o schemă electrică clasică inginerească. Motoarele la deschidere
> sunt de 12V, se vor alimenta de la transformator, și când nu este curent în
> priză de la acumulator 12V 60Ah. Adaugă modul care încarcă acumulatorul și
> asigură alimentare neîntreruptă. Acumulatorul se încarcă sau de la priză sau
> baterie solară."*
>
> *Detalii hardware confirmate:*
> - 2 motoare × 5A fiecare (10A total motoare)
> - Panou solar 100W (aprox. 8A la 12V)
> - Ambele tipuri de scheme (IEC clasică + wiring diagram)

**Sistem UPS proiectat:**
- TR1: Transformator 230V/15V 200VA → punte Graetz BR1 35A → filtru C1 4700µF
- MPPT 10A (Epever/Victron) — intrare max 50V, ieșire 12V
- D6/D7 diode Schottky 20A — separare galvanică rețea ↔ solar (fără ATS)
- A1: Acumulator 12V 60Ah AGM sau LiFePO4
- F4 20A siguranță pe borna + acumulator
- Autonomie estimată la cădere rețea: 48-72h (motoare rar active)

**Schema E01 (IEC inginerească):**
- Format A1 landscape, chenar titlu, marcaje axe (IEC 60617)
- Linie separare circuite forță 12V / circuite comandă
- Simboluri standard: siguranțe, transformator, diode, baterie, motor DC
- BOM 22 componente + 18 note tehnice inginerești

**Schema E02 (Wiring Diagram 12V + UPS Solar):**
- Zone separate cu borduri colorate
- Bară +12V distribuție portocalie + bară GND neagră
- K1/K2 motoare 12V, K3 electrovalvă, K4/K5 ventilatoare
- Buzzer alertă la U_bat < 11.5V

**Fișiere:** `SERA-E01_Schema_IEC_Principala.*` / `SERA-E02_Schema_Wiring_12V_UPS_Final.*`

---

## 10. CERINȚĂ ARHIVĂ COMPLETĂ PENTRU GITHUB

> *„Suplimentar la cele existente, vreau toată arhiva proiectului cu codul sursă
> și documentele pregătite de a fi încărcate pe GitHub."*
>
> *„Vreau tot ce am aici să stochezi pe GitHub în contul meu."*
>
> *„Creează repo sistem automat gestionare seră."*
>
> *„Au fost și versiuni diferite arhivate, vreau să stochezi și acele versiuni,
> citește și cerințele de la început. Fiecare arhivă și documentație generată
> fă-o ca un branch."*

**Repository-uri create pe GitHub (anatolie-ernu):**
- https://github.com/anatolie-ernu/sera-ctrl-01
- https://github.com/anatolie-ernu/sistem-gestionare-sera

**Plan branch-uri (fiecare arhivă = branch separat):**

| Branch | Conținut | Arhivă sursă |
|--------|----------|--------------|
| `main` | Versiunea integrată finală (114 fișiere) | Greenhouse_IoT_Complete.zip |
| `etapa1/firmware-standalone` | Firmware ESP32 fără WiFi | Etapa1_Firmware_ESP32.zip |
| `etapa2/firmware-wifi-mqtt` | Firmware + WiFi/MQTT | Etapa2_Firmware_WiFi_MQTT.zip |
| `etapa3/backend-timescaledb` | Backend complet | Etapa3_Backend_TimescaleDB.zip |
| `etapa6/cicd-gitlab` | CI/CD GitLab | Etapa6_GitLab_CICD.zip |
| `etapa9/security` | Securitate | Etapa9_Securitate.zip |
| `etapa10/e2e-tests` | Teste E2E | Etapa10_Teste_E2E.zip |
| `pas2/productie-saas` | CA + jig + multi-tenant | Pas2_Productie_si_SaaS.zip |
| `pas3/mobile-monitoring` | App mobilă + Prometheus | Pas3_App_Mobila_si_Monitoring.zip |
| `docs/industrializare` | PDF-uri P0-P6 | Industrializare_SERA-CTRL-01.zip |
| `docs/conformitate` | Manual + DoC + Ghid | Pas4_Conformitate_si_Lansare.zip |
| `docs/schema-electrica` | Wiring diagram original | SERA-CTRL-01_Schema_Electrica.* |
| `docs/schema-iec-12v-ups` | Schema IEC 12V + UPS Solar | SERA-E01 + SERA-E02 |

---

## 11. CERINȚĂ FIȘIER CERINȚE

> *„Cerințele pune-le într-un fișier ca cerințe."*

**→ Acest document: `CERINTE.md`**

---

## REZUMAT TEHNIC — STACK FINAL

```
Firmware:      C++ PlatformIO / ESP32-WROOM-32E-N8 / Etapa 1, 2, Production
Backend:       Node.js 20 + Express + TimescaleDB + Redis + Mosquitto
Mobile:        React Native (Expo) — Android + iOS — md.sera.app
Infrastructure: Docker Compose + Nginx SSL + GitHub Actions CI
Monitoring:    Prometheus + Grafana (Fleet Overview dashboard)
PKI:           Root CA (20 ani) + Device CA (10 ani) + X.509 per device
Hardware:      12V DC + TR1 + MPPT 10A + Acumulator 12V 60Ah + PV 100W
Scheme:        E00 Wiring (230V) + E01 IEC principală + E02 Wiring 12V/UPS
```

---

*Generat automat din istoricul conversațiilor Claude AI — Anatolie Ernu, 2026*
