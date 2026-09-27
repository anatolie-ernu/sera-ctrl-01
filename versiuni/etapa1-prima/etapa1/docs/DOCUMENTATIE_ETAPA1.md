# SERA INTELIGENTĂ — ETAPA 1
## Firmware ESP32: Senzori + Actuatoare + Logică Automată Locală

---

## 📋 Cuprins

1. [Obiective Etapă](#obiective)
2. [Hardware Necesar](#hardware)
3. [Instalare Mediu Dezvoltare](#instalare)
4. [Structura Codului](#structura)
5. [Descrierea Fișierelor](#fisiere)
6. [Schema de Cablare](#cablare)
7. [Calibrare Senzori](#calibrare)
8. [Testare și Debugging](#testare)
9. [Comenzi Serial Monitor](#comenzi)
10. [Probleme Cunoscute și Soluții](#probleme)
11. [Ce urmează în Etapa 2](#urmator)

---

## 1. Obiective Etapă {#obiective}

Etapa 1 pune bazele hardware ale proiectului. La finalul acestei etape veți avea:

- ✅ Citire continuă temperatură și umiditate aer (DHT22)
- ✅ Citire continuă umiditate sol (senzor capacitiv analogic)
- ✅ Control complet al celor 5 relee (2 ferestre, 1 pompă, 2 ventilatoare)
- ✅ Logică automată după praguri configurabile (fără server)
- ✅ Interfață de testare prin Serial Monitor
- ✅ Fallback autonom: sera funcționează și fără internet/server

**Nu se implementează în Etapa 1:**
- WiFi și MQTT (Etapa 2)
- Comunicare cu serverul (Etapa 2)
- Scheduler irigare (Etapa 4)
- Display OLED (opțional, Etapa 3)

---

## 2. Hardware Necesar {#hardware}

| Componentă | Specificații | Cantitate | Note |
|---|---|---|---|
| ESP32 DevKit | 38 pini, dual-core 240MHz | 1 | Recomandat: ESP32-WROOM-32 |
| Senzor DHT22 | Temperatură -40~80°C, Umiditate 0~100% | 1 | Mai precis decât DHT11 |
| Senzor sol | Capacitiv (nu rezistiv!) | 1 | Rezistiv se oxidează rapid |
| Modul releu | 5 canale, 5V, 10A/250VAC | 1 | Cu optocuplor izolat |
| Rezistență | 10kΩ, 1/4W | 1 | Pull-up pentru DHT22 |
| Breadboard | Standard 830 puncte | 1 | Pentru prototip |
| Fire dupont | M-M, M-F, F-F | 1 set | Diverse lungimi |
| Alimentare | 5V/2A USB sau adaptor | 1 | Pentru ESP32 |
| Carcasă | IP65, min 15x10x8cm | 1 | Pentru instalare în seră |

**Hardware de acționare (conectat la relee):**
- 2× motoare de fereastră 220V (sau actuatoare liniare)
- 1× robinet electric solenoid 220V
- 2× ventilatoare 220V

---

## 3. Instalare Mediu Dezvoltare {#instalare}

### Pasul 1: Instalare VS Code + PlatformIO

```bash
# 1. Descarcă VS Code de la https://code.visualstudio.com/
# 2. Instalează extensia PlatformIO IDE din marketplace VS Code
# 3. Repornește VS Code
```

### Pasul 2: Clonare/Deschidere proiect

```bash
# Deschide folderul firmware/ în VS Code
# PlatformIO va detecta automat platformio.ini
```

### Pasul 3: Instalare librării

PlatformIO instalează automat librăriile din `platformio.ini`:
- `adafruit/DHT sensor library`
- `adafruit/Adafruit Unified Sensor`

Sau manual:
```
PlatformIO → Libraries → Search "DHT" → Install "DHT sensor library by Adafruit"
```

### Pasul 4: Configurare port COM

Conectează ESP32 prin USB. Verifică portul:
- **Windows:** Device Manager → COM & LPT Ports → `COM3` (sau alt număr)
- **Linux:** `ls /dev/ttyUSB*` → `/dev/ttyUSB0`
- **macOS:** `ls /dev/cu.usbserial*`

Dacă ESP32 nu apare, instalează driverul CP2102 sau CH340 (depinde de chip-ul USB al plăcii tale).

### Pasul 5: Upload firmware

```
PlatformIO → Build → Upload (sau Ctrl+Alt+U)
```

---

## 4. Structura Codului {#structura}

```
firmware/
├── src/
│   ├── main.cpp          ← Punct intrare, setup() și loop()
│   ├── config.h          ← TOATE constantele și pinii
│   ├── sensors.h/.cpp    ← Citire DHT22 și senzor sol
│   ├── actuators.h/.cpp  ← Control relee + logică automată
│   ├── wifi_manager.h    ← Placeholder (Etapa 2)
│   └── display.h         ← Placeholder (opțional, Etapa 3)
├── platformio.ini         ← Configurare PlatformIO
└── schema_conexiuni.txt  ← Schema cablare ASCII
```

**Principiu de design:** Fiecare modul are responsabilitate unică (Single Responsibility). `config.h` este singurul loc unde modifici parametrii — nu e nevoie să cauți prin mai multe fișiere.

---

## 5. Descrierea Fișierelor {#fisiere}

### `config.h` — Configurare centralizată
Toate constantele proiectului. **Modifică DOAR acest fișier** pentru a adapta pragurile și pinii:

```cpp
// Exemplu: vrei să deschizi ferestrele la 30°C în loc de 28°C
#define TEMP_OPEN_WINDOWS   30.0f  // modifică de la 28.0f la 30.0f
```

Variabile cheie:
- `PIN_*` — pinii GPIO
- `TEMP_*` — pragurile de temperatură
- `SOIL_*` — pragurile pentru sol
- `*_INTERVAL_MS` — intervalele de timp
- `RELAY_ACTIVE_LOW` — logica releelor (true pentru majoritatea modulelor)

### `sensors.cpp` — Citire senzori
- `initSensors()` — apelat în `setup()`
- `readAllSensors(data)` — citire DHT22 + soil, populează struct SensorData
- `soilAdcToPercent(adc)` — conversie 0-4095 ADC → 0-100%
- `printSensorData(data)` — output formatat pe Serial

### `actuators.cpp` — Control hardware
- `initActuators()` — setează toți pinii ca OUTPUT, stare inițială OFF
- `openWindows() / closeWindows()` — cu debounce și timp de deplasare
- `startPump() / stopPump()` — cu debounce
- `startFan(n) / stopFan(n)` — control individual ventilator
- `applyThresholdLogic()` — logica automată (apelată din loop)
- `handleSerialCommands()` — interfață testare
- `emergencyStop()` — oprire imediată toate dispozitivele

---

## 6. Schema de Cablare {#cablare}

Vezi fișierul `schema_conexiuni.txt` pentru schema completă ASCII.

### Rezumat pinii ESP32:

| GPIO | Componentă | Direcție |
|------|-----------|----------|
| 4    | DHT22 DATA | INPUT |
| 34   | Soil Sensor AO | INPUT (ADC) |
| 16   | Releu Fereastră 1 | OUTPUT |
| 17   | Releu Fereastră 2 | OUTPUT |
| 18   | Releu Pompă | OUTPUT |
| 19   | Releu Fan 1 | OUTPUT |
| 21   | Releu Fan 2 | OUTPUT |
| 2    | LED Status | OUTPUT |

> ⚠️ **GPIO 34, 35, 36, 39** pe ESP32 sunt INPUT-ONLY — nu pot fi configurate ca OUTPUT.

---

## 7. Calibrare Senzori {#calibrare}

### Calibrare senzor sol (OBLIGATORIE)

Valorile ADC variază între senzori. Trebuie să-ți calibrezi senzorul specific:

**Pasul 1:** Pune senzorul în aer (simulează sol complet uscat)
```
Serial Monitor → citești ceva de genul: Sol (ADC): 3150
```
Aceasta este valoarea `ADC_DRY` → actualizează în `sensors.cpp`:
```cpp
const int ADC_DRY = 3150;  // valoarea ta
```

**Pasul 2:** Pune senzorul în pahar cu apă (simulează sol complet ud)
```
Serial Monitor → citești ceva de genul: Sol (ADC): 820
```
Aceasta este valoarea `ADC_WET`:
```cpp
const int ADC_WET = 820;  // valoarea ta
```

### Verificare DHT22

DHT22 este calibrat din fabrică. Dacă valorile par incorecte:
- Verifică rezistența pull-up 10kΩ
- Verifică că firele nu sunt prea lungi (max 10m)
- Așteaptă minim 2 secunde după pornire înainte de prima citire

---

## 8. Testare și Debugging {#testare}

### Test 1: Verificare senzori (fără relee conectate)

1. Upload firmware
2. Deschide Serial Monitor (115200 baud)
3. Aștepți 30 secunde pentru prima citire
4. Verifici că valorile sunt rezonabile:
   - Temperatură: 15-35°C (tipic seră)
   - Umiditate: 40-90%
   - Sol: valoare ADC brută + procent

### Test 2: Verificare relee (FĂRĂ 220V conectat!)

Trimite comenzi prin Serial Monitor:
```
PUMP_ON     → LED-ul releului 3 se aprinde
PUMP_OFF    → LED-ul releului 3 se stinge
FAN1_ON     → LED-ul releului 4 se aprinde
STATUS      → afișează starea tuturor dispozitivelor
```

### Test 3: Logică automată

1. Activează `AUTO_OFF` pentru a dezactiva logica automată
2. Simulează temperaturi modificând `TEMP_OPEN_WINDOWS` la o valoare sub temperatura curentă
3. Activează `AUTO_ON` și observă comportamentul

---

## 9. Comenzi Serial Monitor {#comenzi}

Deschide Serial Monitor la 115200 baud și trimite:

| Comandă | Efect |
|---------|-------|
| `HELP` | Listează toate comenzile |
| `STATUS` | Afișează starea tuturor dispozitivelor |
| `OPEN_WINDOWS` | Deschide ferestrele manual |
| `CLOSE_WINDOWS` | Închide ferestrele manual |
| `PUMP_ON` | Pornește pompa manual |
| `PUMP_OFF` | Oprește pompa manual |
| `FAN1_ON` | Pornește ventilatorul 1 |
| `FAN1_OFF` | Oprește ventilatorul 1 |
| `FAN2_ON` | Pornește ventilatorul 2 |
| `FAN2_OFF` | Oprește ventilatorul 2 |
| `AUTO_ON` | Activează modul automat |
| `AUTO_OFF` | Dezactivează modul automat |
| `STOP_ALL` | Oprire urgență - tot oprit |

---

## 10. Probleme Cunoscute și Soluții {#probleme}

### DHT22 returnează `nan`
**Cauze posibile:**
1. Lipsă rezistență pull-up 10kΩ → adaugă rezistența
2. Pin greșit în `config.h` → verifică `PIN_DHT22`
3. Senzor defect → testează cu alt senzor

### Releele nu se activează
**Cauze posibile:**
1. `RELAY_ACTIVE_LOW` incorect → încearcă `false` dacă e `true` și viceversa
2. Alimentare insuficientă → modulul releu necesită 5V stabil, minim 500mA
3. Pin greșit → verifică `PIN_RELAY_*` în `config.h`

### Senzorul sol dă valori constante
**Cauze posibile:**
1. Pin GPIO 34 — verifică că e conectat la pinul ADC corect
2. Senzor rezistiv oxidat → înlocuiește cu senzor capacitiv
3. Tensiune incorectă → unele senzoare necesită 5V nu 3.3V

### ESP32 se resetează aleatoriu (watchdog reset)
**Cauze posibile:**
1. `delay(WINDOW_TRAVEL_TIME_MS)` blochează taskul WiFi → normal în Etapa 1, se rezolvă în Etapa 2 cu FreeRTOS tasks
2. Alimentare instabilă → folosiți un condensator 100µF între 5V și GND

---

## 11. Ce urmează în Etapa 2 {#urmator}

Etapa 2 adaugă:
- **WiFi Manager** cu portal de configurare (fără hardcoding SSID/parolă)
- **MQTT Client** (PubSubClient) pentru comunicare bidirecțională cu serverul
- **Publicare date senzori** pe topic `greenhouse/sensors` la fiecare 30s
- **Recepție comenzi** de pe topic `greenhouse/cmd/*`
- **Heartbeat** (ping la fiecare 60s pentru a detecta deconectarea)
- **Reconnect logic** — reconectare automată la WiFi și MQTT
- **NTP time sync** — sincronizare oră exactă de pe internet

---

## Resurse Utile

- [Documentație ESP32 oficială](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [PlatformIO ESP32](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
- [DHT Library Adafruit](https://github.com/adafruit/DHT-sensor-library)
- [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)

---

*Document generat pentru: Sera Inteligentă v1.0 | Etapa 1 din 10*
