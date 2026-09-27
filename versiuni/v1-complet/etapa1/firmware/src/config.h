/**
 * config.h — Sera Inteligenta Etapa 1
 * SINGURUL fisier pe care il modifici pentru a adapta sistemul!
 */
#pragma once

#define FIRMWARE_VERSION    "1.0.0"
#define DEVICE_ID           "SERA_001"

// ── Pini ESP32 ────────────────────────────────────────────────
#define PIN_DHT22           4
#define PIN_SOIL_SENSOR     34   // ADC-only
#define PIN_RELAY_WINDOW1   16
#define PIN_RELAY_WINDOW2   17
#define PIN_RELAY_PUMP      18
#define PIN_RELAY_FAN1      19
#define PIN_RELAY_FAN2      21
#define PIN_LED_STATUS      2
#define DHT_TYPE            DHT22

// ── Intervale (ms) ────────────────────────────────────────────
#define SENSOR_INTERVAL_MS       30000UL
#define RELAY_DEBOUNCE_MS        2000UL
#define WINDOW_TRAVEL_TIME_MS    15000UL

// ── Praguri temperatura (°C) ─────────────────────────────────
#define TEMP_OPEN_WINDOWS    28.0f
#define TEMP_CLOSE_WINDOWS   24.0f
#define TEMP_START_FANS      26.0f
#define TEMP_STOP_FANS       23.0f
#define TEMP_ALERT_HIGH      35.0f
#define TEMP_ALERT_LOW        5.0f

// ── Praguri umiditate (%) ────────────────────────────────────
#define HUMIDITY_ALERT_HIGH  90.0f
#define HUMIDITY_ALERT_LOW   30.0f

// ── Calibrare senzor sol (ADC 12-bit: 0-4095) ────────────────
// Ruleaza: ./scripts/calibrate.sh pentru valorile tale
#define SOIL_ADC_DRY         3200    // in aer  = 0%
#define SOIL_ADC_WET          800    // in apa  = 100%
#define SOIL_ALERT_DRY_PCT   20.0f

// ── Logica relee ──────────────────────────────────────────────
#define RELAY_ACTIVE_LOW    true
#define RELAY_ON            (RELAY_ACTIVE_LOW ? LOW  : HIGH)
#define RELAY_OFF           (RELAY_ACTIVE_LOW ? HIGH : LOW)

// ── Structuri de date ─────────────────────────────────────────
struct SensorData {
    float temperature, humidity;
    int   soilRaw;
    float soilPct;
    bool  dhtValid, soilValid;
    unsigned long timestamp;
};

struct SystemState {
    bool windowsOpen, pumpRunning, fan1Running, fan2Running;
    bool autoMode, alertActive;
    unsigned long pumpStartTime, pumpDuration;
};
