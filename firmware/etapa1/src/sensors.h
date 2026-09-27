/**
 * sensors.h  —  Greenhouse IoT System | Firmware Stage 1
 * ─────────────────────────────────────────────────────────────────────────────
 * Public interface for the sensor module.
 * Include this header in any file that needs to read sensor data.
 *
 * Implemented in: sensors.cpp
 * Depends on:     config.h, DHT library (Adafruit)
 */
#pragma once
#include "config.h"

/** Initialise the DHT22 and configure the ADC pin. Call once in setup(). */
void initSensors();

/**
 * Read all sensors and populate the SensorData struct.
 * Always call this before reading d.temperature etc.
 * If a sensor fails, the corresponding *Valid flag is set false.
 */
void readAllSensors(SensorData &d);

/** Print a formatted sensor reading to Serial (115200 baud). */
void printSensorData(const SensorData &d);

/**
 * Convert raw 12-bit ADC reading to moisture percentage (0–100 %).
 * Uses linear interpolation between SOIL_ADC_DRY (= 0 %) and
 * SOIL_ADC_WET (= 100 %).  Values outside the calibration range are clamped.
 */
float soilAdcToPercent(int adc);

/** Return a human-readable soil status string for logging. */
const char* getSoilStatus(float pct);

/**
 * Serialise the current sensor readings and system state to a compact
 * JSON string.  Used by Stage 2 to publish data over MQTT.
 * Returns a String object (heap-allocated; ~200 bytes typical).
 */
String sensorDataToJson(const SensorData &d, const SystemState &s);
