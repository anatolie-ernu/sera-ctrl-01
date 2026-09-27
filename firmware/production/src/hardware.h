/**
 * hardware.h — Pin map PCB producție SERA-CTRL-01 rev.B
 * Toate GPIO-urile corespund schemei electrice (schema_electrica.svg)
 */
#pragma once

// ── Relee (prin ULN2003A) ────────────────────────────────────
#define PIN_K1_WIN1   16   // Fereastră 1
#define PIN_K2_WIN2   17   // Fereastră 2
#define PIN_K3_PUMP   18   // Electrovalvă irigare
#define PIN_K4_FAN1   19   // Ventilator 1
#define PIN_K5_FAN2   23   // Ventilator 2

// ── Senzori ──────────────────────────────────────────────────
#define PIN_SDA       21   // I2C SDA → SHT31
#define PIN_SCL       22   // I2C SCL → SHT31
#define PIN_SOIL_ADC  34   // ADC1_CH6, input-only (senzor sol)

// ── RS485 (Modbus extern, opțional) ─────────────────────────
#define PIN_RS485_TX  25
#define PIN_RS485_RX  26
#define PIN_RS485_DE  27   // Driver Enable

// ── LED-uri panou UI ─────────────────────────────────────────
#define PIN_LED_PWR   2    // Verde  — alimentat
#define PIN_LED_WIFI  4    // Albastru — WiFi conectat
#define PIN_LED_ST    5    // Galben — status / alertă

// ── Control ──────────────────────────────────────────────────
#define PIN_BTN_BOOT  0    // Buton BOOT: 5s=provisioning, 10s=factory reset

// ── Constante aplicație ──────────────────────────────────────
#define FW_VERSION    "3.0.0"
#define HW_REVISION   "B"
#define RELAY_COUNT   5
#define WATCHDOG_S    45
