/**
 * ============================================================
 * SERA INTELIGENTA - ETAPA 1
 * Fișier: config.h
 * Descriere: Toate constantele, pinii și pragurile sistemului
 * ============================================================
 */

#pragma once

// ─── Versiune firmware ────────────────────────────────────────
#define FIRMWARE_VERSION  "1.0.0"
#define DEVICE_ID         "SERA_001"

// ─── Pinii ESP32 ──────────────────────────────────────────────
// Senzori
#define PIN_DHT22         4    // Senzor temperatură/umiditate aer
#define PIN_SOIL_SENSOR   34   // Senzor umiditate sol (ADC - intrare analogică)

// Relee actuatoare (LOW = activat pentru releele tipice)
#define PIN_RELAY_WINDOW1 16   // Motor fereastră 1
#define PIN_RELAY_WINDOW2 17   // Motor fereastră 2
#define PIN_RELAY_PUMP    18   // Robinet electric 220V (pompă apă)
#define PIN_RELAY_FAN1    19   // Ventilator 1
#define PIN_RELAY_FAN2    21   // Ventilator 2

// LED status (opțional - LED intern ESP32)
#define PIN_LED_STATUS    2

// ─── Tipul senzorului DHT ─────────────────────────────────────
#define DHT_TYPE          DHT22   // Sau DHT11 dacă folosești alt model

// ─── Intervale de timp (milisecunde) ─────────────────────────
#define SENSOR_INTERVAL_MS      30000   // Citire senzori la 30 secunde
#define RELAY_DEBOUNCE_MS       2000    // Pauză minimă între comutări releu
#define WINDOW_TRAVEL_TIME_MS   15000   // Timp deschidere/închidere completă fereastră

// ─── Praguri temperatură (°C) ────────────────────────────────
#define TEMP_OPEN_WINDOWS       28.0f   // Deschide ferestrele când T > 28°C
#define TEMP_CLOSE_WINDOWS      24.0f   // Închide ferestrele când T < 24°C
#define TEMP_START_FANS         26.0f   // Pornește ventilatoarele când T > 26°C
#define TEMP_STOP_FANS          23.0f   // Oprește ventilatoarele când T < 23°C
#define TEMP_ALERT_HIGH         35.0f   // Alertă critică temperatură prea mare
#define TEMP_ALERT_LOW          5.0f    // Alertă critică temperatură prea mică

// ─── Praguri umiditate aer (%) ───────────────────────────────
#define HUMIDITY_ALERT_HIGH     90.0f   // Alertă umiditate prea mare
#define HUMIDITY_ALERT_LOW      30.0f   // Alertă umiditate prea mică

// ─── Praguri umiditate sol ────────────────────────────────────
// Valorile ADC 0-4095 pe ESP32 (12-bit)
// Calibrare: sol uscat ~3000, sol ud ~1000
#define SOIL_DRY_THRESHOLD      2500    // Sub această valoare = sol uscat
#define SOIL_WET_THRESHOLD      1500    // Peste această valoare = sol ud
#define SOIL_ALERT_DRY          2800    // Alertă sol foarte uscat

// ─── Logică relee ────────────────────────────────────────────
// Setează RELAY_ACTIVE_LOW true dacă releele tale se activează pe LOW
#define RELAY_ACTIVE_LOW        true
#define RELAY_ON                (RELAY_ACTIVE_LOW ? LOW  : HIGH)
#define RELAY_OFF               (RELAY_ACTIVE_LOW ? HIGH : LOW)

// ─── Structuri de date ───────────────────────────────────────
struct SensorData {
  float   temperature;      // °C
  float   humidity;         // %
  int     soilMoisture;     // valoare ADC brută 0-4095
  float   soilMoisturePct;  // procent calculat 0-100%
  bool    dhtValid;         // true dacă citirea DHT este validă
  bool    soilValid;        // true dacă citirea solului este validă
  unsigned long timestamp;  // millis() la momentul citirii
};

struct SystemState {
  bool windowsOpen;
  bool pumpRunning;
  bool fan1Running;
  bool fan2Running;
  bool autoMode;            // true = automat, false = manual
  bool alertActive;
};
