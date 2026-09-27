/**
 * ============================================================
 * SERA INTELIGENTA - ETAPA 1: Firmware ESP32
 * Fișier: main.cpp
 * Descriere: Punct de intrare principal - inițializare și loop
 * ============================================================
 */

#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "actuators.h"
#include "wifi_manager.h"
#include "display.h"

// ─── Variabile globale de stare ───────────────────────────────
SensorData sensorData;
SystemState systemState;

unsigned long lastSensorRead  = 0;
unsigned long lastStatusPrint = 0;

// ─── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("╔════════════════════════════════════╗");
  Serial.println("║   SERA INTELIGENTA v1.0 - ETAPA 1  ║");
  Serial.println("╚════════════════════════════════════╝");

  // Inițializare actuatoare (relee, pompă)
  initActuators();

  // Inițializare senzori (DHT22, soil)
  initSensors();

  // Stare implicită: totul oprit
  systemState.windowsOpen    = false;
  systemState.pumpRunning    = false;
  systemState.fan1Running    = false;
  systemState.fan2Running    = false;
  systemState.autoMode       = true;  // Modul automat activat implicit

  Serial.println("[SYSTEM] Inițializare completă.");
  Serial.println("[SYSTEM] Mod automat ACTIV.");
}

// ─── Loop principal ───────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── Citire senzori la fiecare SENSOR_INTERVAL ms ──
  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = now;
    readAllSensors(sensorData);
    printSensorData(sensorData);
  }

  // ── Logică automată după praguri ──
  if (systemState.autoMode) {
    applyThresholdLogic(sensorData, systemState);
  }

  // ── Verificare comenzi seriale (pentru testare manuală) ──
  handleSerialCommands(systemState);

  delay(100);
}
