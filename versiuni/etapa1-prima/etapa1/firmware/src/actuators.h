/**
 * ============================================================
 * SERA INTELIGENTA - ETAPA 1
 * Fișier: actuators.h
 * Descriere: Control relee - ferestre, pompă, ventilatoare
 * ============================================================
 */

#pragma once
#include "config.h"

// ─── Inițializare ─────────────────────────────────────────────
void initActuators();

// ─── Control ferestre ─────────────────────────────────────────
void openWindows();
void closeWindows();
bool areWindowsOpen();

// ─── Control pompă / robinet ──────────────────────────────────
void startPump();
void stopPump();
bool isPumpRunning();

// ─── Control ventilatoare ─────────────────────────────────────
void startFan(uint8_t fanNumber);   // fanNumber: 1 sau 2
void stopFan(uint8_t fanNumber);
void startAllFans();
void stopAllFans();

// ─── Logică automată după praguri ────────────────────────────
void applyThresholdLogic(const SensorData &data, SystemState &state);

// ─── Comenzi seriale (pentru testare) ────────────────────────
void handleSerialCommands(SystemState &state);

// ─── Oprire de urgență - tot off ─────────────────────────────
void emergencyStop();
