# SERA-CTRL-01 rev A — Specificație Schemă Electrică
## Document pentru inginerul PCB / serviciul de proiectare

> Acest document descrie complet fiecare bloc funcțional cu componente și
> valori, suficient pentru implementarea directă în KiCad/Altium.
> Toate codurile LCSC sunt verificabile pe lcsc.com.

---

## BLOC 1 — Intrare AC și Alimentare

### 1.1 Intrare 230V AC
| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| J1  | Terminal bloc 2P, 5.08mm | DG500-5.08-02P | C707593 | L + N, 16A rated |
| F1  | Siguranță 5x20 + suport | 2A T (slow-blow) | C3265 | Pe linia L, înainte de tot |
| RV1 | Varistor MOV | 10D471K (470V) | C129863 | Paralel L-N, după F1 |
| —   | Termofuzibil opțional | 102°C | — | În serie cu RV1 (protecție MOV scurt) |

### 1.2 Sursă AC-DC primară
| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| PS1 | Modul AC-DC izolat | **Hi-Link HLK-10M05** (5V/2A, 10W) | C209901 | Certificat CE/UL; alternativă premium: Mean Well IRM-10-5 |
| C1  | Electrolitic | 470µF/16V low-ESR | C134811 | Pe ieșirea 5V |
| C2  | Ceramic | 100nF/50V X7R | C49678 | Decuplare |

**⚠ Distanță critică:** zona primară 230V trebuie separată de zona SELV cu
**minim 6.4mm creepage** (IEC 62368-1, poluare gradul 2, 250V working).
Frezați un **slot în PCB** sub PS1 între primar și secundar.

### 1.3 Regulator 3.3V pentru ESP32
| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| U2  | LDO 800mA | ME6217C33M5G | C427602 | Dropout mic; ESP32 are vârfuri 500mA la TX WiFi |
| C3  | Ceramic | 10µF/16V | C19702 | Intrare U2 |
| C4  | Ceramic | 22µF/10V | C59461 | Ieșire U2 — esențial pentru stabilitate WiFi |

---

## BLOC 2 — Microcontroler ESP32

| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| U1  | Modul WiFi | **ESP32-WROOM-32E-N8** (8MB) | C701343 | Pre-certificat RED/FCC — reduce drastic costul certificării! |
| C5  | Ceramic | 10µF + 100nF | — | La pin 3V3, cât mai aproape |
| R1  | Rezistor | 10kΩ | C25804 | Pull-up EN |
| C6  | Ceramic | 1µF | C15849 | EN la GND (delay reset) |
| SW1 | Buton tact 6x6 | TS-1187A | C318884 | BOOT (GPIO0) — provisioning/factory reset |
| SW2 | Buton tact 6x6 | TS-1187A | C318884 | RESET (EN) |

**Zonă de keep-out antenă:** NU plasați cupru/componente sub antena modulului
(zona marcată pe datasheet-ul Espressif) și mențineți antena la marginea PCB.

### 2.1 Programare USB
| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| J2  | USB-C 16 pini | TYPE-C-31-M-12 | C165948 | Doar pentru programare/debug |
| U3  | USB-UART | **CP2102N-A02-GQFN24** | C964632 | Driver universal, stabil |
| Q1,Q2 | NPN | S8050 | C2146 | Circuit auto-program standard (DTR/RTS → EN/IO0) |
| R2,R3 | Rezistor | 10kΩ ×2 | C25804 | Bazele Q1/Q2 |
| R4,R5 | Rezistor | 5.1kΩ ×2 | C23186 | CC1/CC2 USB-C la GND |

---

## BLOC 3 — Ieșiri Releu (5 canale)

| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| U4  | Driver | **ULN2003A** (SOIC-16) | C7512 | 7 canale Darlington, diode flyback integrate |
| K1–K5 | Releu | **Hongfa HF115F/005-1ZS3** | C190253 | 12A/250VAC, bobină 5V — adecvat motoare inductive |
| LED1–5 | LED 0805 verde | — | C2297 | Indicator per canal + R 1kΩ |
| RC1–RC5 | Snubber RC | 100Ω/2W + 100nF/275V X2 | C129864 | Peste contactele K1,K2 (motoare) — suprimă arcul |
| J3–J7 | Terminal 3P 5.08mm | DG500-5.08-03P | C707594 | COM/NO/NC per releu |

**Logică:** GPIO ESP32 → intrare ULN2003A → bobină releu la 5V.
ULN2003A e ACTIVE-HIGH → în firmware `RELAY_ACTIVE_LOW = false`.

**Layout 230V:** trasee contacte releu — **lățime min 3mm la 2oz cupru**
pentru 12A; mențineți 6.4mm creepage față de orice traseu SELV; sloturi
frezate între canalele adiacente de releu.

---

## BLOC 4 — Interfețe Senzori

### 4.1 SHT31 (temperatură/umiditate — înlocuiește DHT22)
| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| J8  | Conector M12 4-pini sau JST-XH 4P | — | C158012 | 3V3/GND/SDA/SCL |
| R6,R7 | Pull-up I2C | 4.7kΩ ×2 | C23162 | Pe SDA/SCL |
| D1,D2 | TVS | PESD3V3L2BT ×2 | C448739 | ESD pe liniile I2C |

Senzorul SHT31 se montează pe un mic PCB satelit în sondă ventilată,
cablu ecranat max 3m. Pentru distanțe mai mari → folosiți portul RS485.

### 4.2 Intrare sol analogică (compatibilitate senzori capacitivi)
| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| J9  | JST-XH 3P | — | C158012 | 3V3/GND/AOUT |
| R8  | Serie | 1kΩ | C21190 | Protecție GPIO34 |
| C7  | Filtru | 100nF | C49678 | RC low-pass cu R8 |
| D3  | TVS | PESD3V3L1BA | C456094 | Clampare la 3.3V |

### 4.3 Port RS485 (senzori industriali Modbus — diferențiator de produs!)
| Ref | Componentă | Valoare/Cod | LCSC | Notă |
|-----|-----------|-------------|------|------|
| U5  | Transceiver | **MAX3485ESA** (3.3V) | C9979 | Half-duplex, DE pe GPIO27 |
| R9  | Terminator | 120Ω (jumper selectabil) | C17909 | Capăt de magistrală |
| D4  | TVS bidirect. | SM712 | C12673 | Protecție A/B |
| J10 | Terminal 3P | A/B/GND | C707594 | Magistrală RS485 |

---

## BLOC 5 — UI Panou
| Ref | Componentă | Notă |
|-----|-----------|------|
| LED_PWR (verde, GPIO2) | Alimentare OK |
| LED_WIFI (albastru, GPIO4) | Fix=conectat, clipire=conectare/provisioning |
| LED_ST (galben, GPIO5) | MQTT OK / pompă activă |
| Lumină buton BOOT vizibilă/accesibilă prin carcasă | provisioning + factory reset |

---

## Conexiuni GPIO — rezumat (sincronizat cu firmware hardware.h)

| GPIO | Funcție | GPIO | Funcție |
|------|---------|------|---------|
| 0  | BTN_BOOT | 21 | I2C SDA |
| 2  | LED_PWR  | 22 | I2C SCL |
| 4  | LED_WIFI | 23 | Releu K5 (FAN2) |
| 5  | LED_ST   | 25 | RS485 TX |
| 16 | Releu K1 (WIN1) | 26 | RS485 RX |
| 17 | Releu K2 (WIN2) | 27 | RS485 DE |
| 18 | Releu K3 (PUMP) | 34 | Sol ADC (input-only) |
| 19 | Releu K4 (FAN1) | | |
