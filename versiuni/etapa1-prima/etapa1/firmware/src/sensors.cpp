/**
 * ============================================================
 * SERA INTELIGENTA - ETAPA 1
 * Fișier: sensors.cpp
 * Descriere: Implementarea citirii senzorilor DHT22 și soil
 * ============================================================
 */

#include "sensors.h"
#include <DHT.h>

// Instanță globală DHT
static DHT dht(PIN_DHT22, DHT_TYPE);

// ─── Inițializare ─────────────────────────────────────────────
void initSensors() {
  dht.begin();

  // Soil sensor pe pin ADC - configurare input
  pinMode(PIN_SOIL_SENSOR, INPUT);

  Serial.println("[SENSORS] DHT22 inițializat pe pinul " + String(PIN_DHT22));
  Serial.println("[SENSORS] Senzor sol inițializat pe pinul " + String(PIN_SOIL_SENSOR));

  delay(2000); // DHT22 are nevoie de 2s după pornire pentru stabilizare
  Serial.println("[SENSORS] Senzorii sunt gata.");
}

// ─── Citire toți senzorii ─────────────────────────────────────
void readAllSensors(SensorData &data) {
  data.timestamp = millis();

  // ── DHT22 ──
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("[SENSORS] EROARE: Citire DHT22 eșuată!");
    data.dhtValid    = false;
    data.temperature = 0.0f;
    data.humidity    = 0.0f;
  } else {
    data.temperature = t;
    data.humidity    = h;
    data.dhtValid    = true;
  }

  // ── Senzor sol (analogic) ──
  // ESP32: analogRead returnează 0-4095 (12-bit ADC)
  int rawSoil = analogRead(PIN_SOIL_SENSOR);

  if (rawSoil < 0 || rawSoil > 4095) {
    Serial.println("[SENSORS] EROARE: Citire senzor sol eșuată!");
    data.soilValid       = false;
    data.soilMoisture    = 0;
    data.soilMoisturePct = 0.0f;
  } else {
    data.soilMoisture    = rawSoil;
    data.soilMoisturePct = soilAdcToPercent(rawSoil);
    data.soilValid       = true;
  }
}

// ─── Conversie ADC → procente sol ────────────────────────────
// ADC maxim (uscat) ~3200 = 0%, ADC minim (ud) ~800 = 100%
// Calibrează aceste valori după senzorul tău specific!
float soilAdcToPercent(int adcValue) {
  const int ADC_DRY = 3200;   // Valoare în aer / sol complet uscat
  const int ADC_WET = 800;    // Valoare în apă / sol complet ud

  // Clampare la limitele de calibrare
  if (adcValue >= ADC_DRY) return 0.0f;
  if (adcValue <= ADC_WET)  return 100.0f;

  // Interpolare liniară inversă (mai mic ADC = mai ud)
  float pct = (float)(ADC_DRY - adcValue) / (float)(ADC_DRY - ADC_WET) * 100.0f;
  return pct;
}

// ─── Status textual sol ───────────────────────────────────────
const char* getSoilStatus(float pct) {
  if (pct < 20.0f) return "USCAT CRITIC";
  if (pct < 40.0f) return "USCAT";
  if (pct < 60.0f) return "OPTIM";
  if (pct < 80.0f) return "UMED";
  return "SATURAT";
}

// ─── Afișare date pe Serial ───────────────────────────────────
void printSensorData(const SensorData &data) {
  Serial.println("┌─────────────────────────────────────┐");
  Serial.println("│         DATE SENZORI                │");
  Serial.println("├─────────────────────────────────────┤");

  if (data.dhtValid) {
    Serial.printf("│ Temperatura:  %6.1f °C             │\n", data.temperature);
    Serial.printf("│ Umiditate:    %6.1f %%              │\n", data.humidity);
  } else {
    Serial.println("│ Temperatura:  EROARE CITIRE         │");
    Serial.println("│ Umiditate:    EROARE CITIRE         │");
  }

  if (data.soilValid) {
    Serial.printf("│ Sol (ADC):    %6d               │\n", data.soilMoisture);
    Serial.printf("│ Sol (%%):      %6.1f %%  [%s]  │\n",
                  data.soilMoisturePct,
                  getSoilStatus(data.soilMoisturePct));
  } else {
    Serial.println("│ Sol:          EROARE CITIRE         │");
  }

  Serial.printf("│ Timestamp:    %lu ms            │\n", data.timestamp);
  Serial.println("└─────────────────────────────────────┘");
}
