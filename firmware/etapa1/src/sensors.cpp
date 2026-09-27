/**
 * sensors.cpp  —  Greenhouse IoT System | Firmware Stage 1
 * ─────────────────────────────────────────────────────────────────────────────
 * Implements sensor initialisation and data acquisition for:
 *   • DHT22 — temperature and relative humidity (digital, 1-wire protocol)
 *   • Capacitive soil moisture sensor — analog voltage → 12-bit ADC
 *
 * Design notes
 * ─────────────────────────────────────────────────────────────────────────────
 * DHT22 minimum sample interval is 2 seconds; the firmware polls every
 * SENSOR_INTERVAL_MS (30 s by default) so this constraint is always satisfied.
 *
 * The soil sensor's analog output is read on GPIO 34, which belongs to ADC1.
 * ADC2 pins on the ESP32 are unavailable when WiFi is active (Stage 2), so
 * always use ADC1 pins (32–39) for analog sensors.
 *
 * The soilAdcToPercent() mapping is INVERSE: a higher raw ADC value means
 * less moisture.  Values outside [SOIL_ADC_WET, SOIL_ADC_DRY] are clamped to
 * the [0, 100] range so the output is always a valid percentage.
 */
#include "sensors.h"
#include <DHT.h>
#include <Arduino.h>

// DHT library instance — created once at module scope (not on the stack)
static DHT dht(PIN_DHT22, DHT_TYPE);

// ─────────────────────────────────────────────────────────────────────────────
void initSensors() {
    dht.begin();                     // Start DHT22 1-wire communication
    pinMode(PIN_SOIL_SENSOR, INPUT); // Configure ADC pin as input (default, but explicit)

    Serial.printf("[SENSORS] DHT22 on GPIO%d | Soil on GPIO%d\n",
                  PIN_DHT22, PIN_SOIL_SENSOR);

    // DHT22 needs ~2 s after power-on before the first valid reading
    delay(2000);
    Serial.println("[SENSORS] Ready.");
}

// ─────────────────────────────────────────────────────────────────────────────
void readAllSensors(SensorData &d) {
    d.timestamp = millis();  // Record when this reading was taken

    // ── DHT22 ─────────────────────────────────────────────────────────────
    // readTemperature() and readHumidity() return NaN on failure.
    // isnan() catches both NaN and inf.
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    d.dhtValid    = !(isnan(t) || isnan(h));
    d.temperature = d.dhtValid ? t : 0.0f;
    d.humidity    = d.dhtValid ? h : 0.0f;

    if (!d.dhtValid) Serial.println("[SENSORS] ERROR: DHT22 read failed!");

    // ── Soil sensor (analog) ───────────────────────────────────────────────
    // analogRead() returns 0–4095 for ESP32 12-bit ADC.
    // Sanity check: reject values exactly 0 or 4095 which usually indicate a
    // wiring fault (sensor disconnected → ADC floats to 0 or rail).
    int raw = analogRead(PIN_SOIL_SENSOR);

    // Accept values in 1–4094; reject 0 and 4095 as sensor-fault indicators
    d.soilValid = (raw > 0 && raw < 4095);
    d.soilRaw   = d.soilValid ? raw : 0;
    d.soilPct   = d.soilValid ? soilAdcToPercent(raw) : 0.0f;

    if (!d.soilValid)
        Serial.printf("[SENSORS] ERROR: Soil ADC=%d (check wiring!)\n", raw);
}

// ─────────────────────────────────────────────────────────────────────────────
float soilAdcToPercent(int adc) {
    // Clamp to calibration range first
    if (adc >= SOIL_ADC_DRY) return 0.0f;    // As dry as sensor can detect
    if (adc <= SOIL_ADC_WET) return 100.0f;  // As wet as sensor can detect

    // Linear interpolation (inverse: lower ADC = higher moisture)
    // Formula: pct = (DRY - adc) / (DRY - WET) * 100
    return (float)(SOIL_ADC_DRY - adc)
         / (float)(SOIL_ADC_DRY - SOIL_ADC_WET)
         * 100.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
const char* getSoilStatus(float pct) {
    if (pct < 20.0f) return "DRY-CRITICAL";
    if (pct < 40.0f) return "DRY";
    if (pct < 60.0f) return "OPTIMAL";
    if (pct < 80.0f) return "WET";
    return "SATURATED";
}

// ─────────────────────────────────────────────────────────────────────────────
void printSensorData(const SensorData &d) {
    Serial.println("┌──────────────────────────────────┐");
    if (d.dhtValid) {
        Serial.printf("│ Temp:  %6.1f °C               │\n", d.temperature);
        Serial.printf("│ Humid: %6.1f %%                │\n", d.humidity);
    } else {
        Serial.println("│ Temp/Humid: READ ERROR          │");
    }
    if (d.soilValid) {
        Serial.printf("│ Soil ADC:  %5d              │\n", d.soilRaw);
        Serial.printf("│ Soil %%:    %5.1f  [%-13s]│\n",
                      d.soilPct, getSoilStatus(d.soilPct));
    } else {
        Serial.println("│ Soil: READ ERROR (check wiring) │");
    }
    Serial.printf("│ Uptime: %lu ms            │\n", d.timestamp);
    Serial.println("└──────────────────────────────────┘");
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON serialisation — used by Stage 2 MQTT publisher.
// snprintf into a fixed buffer is safer than String concatenation (no heap
// fragmentation) and deterministic in timing.
String sensorDataToJson(const SensorData &d, const SystemState &s) {
    char buf[320];
    snprintf(buf, sizeof(buf),
        "{"
        "\"device\":\"%s\","
        "\"temp\":%.1f,\"hum\":%.1f,"
        "\"soil_raw\":%d,\"soil_pct\":%.1f,"
        "\"dht_ok\":%s,\"soil_ok\":%s,"
        "\"state\":{"
            "\"windows\":%s,"
            "\"pump\":%s,"
            "\"fan1\":%s,"
            "\"fan2\":%s,"
            "\"auto\":%s"
        "},"
        "\"ts\":%lu"
        "}",
        DEVICE_ID,
        d.temperature, d.humidity,
        d.soilRaw, d.soilPct,
        d.dhtValid  ? "true" : "false",
        d.soilValid ? "true" : "false",
        s.windowsOpen  ? "true" : "false",
        s.pumpRunning  ? "true" : "false",
        s.fan1Running  ? "true" : "false",
        s.fan2Running  ? "true" : "false",
        s.autoMode     ? "true" : "false",
        d.timestamp
    );
    return String(buf);
}
