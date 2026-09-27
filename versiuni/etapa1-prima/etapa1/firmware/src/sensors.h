/**
 * ============================================================
 * SERA INTELIGENTA - ETAPA 1
 * Fișier: sensors.h
 * Descriere: Interfață pentru citirea senzorilor
 * ============================================================
 */

#pragma once
#include "config.h"

// Inițializează toți senzorii
void initSensors();

// Citește toți senzorii și populează structura SensorData
void readAllSensors(SensorData &data);

// Afișează datele pe Serial
void printSensorData(const SensorData &data);

// Convertește valoarea ADC a solului în procente (0-100%)
float soilAdcToPercent(int adcValue);

// Returnează descrierea stării solului
const char* getSoilStatus(float pct);
