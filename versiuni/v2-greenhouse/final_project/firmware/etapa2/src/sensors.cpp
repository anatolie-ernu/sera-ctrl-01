#include "sensors.h"
#include <DHT.h>
#include <Arduino.h>

static DHT dht(PIN_DHT22, DHT_TYPE);

void initSensors() {
    dht.begin();
    pinMode(PIN_SOIL_SENSOR, INPUT);
    Serial.printf("[SENSORS] DHT22->GPIO%d  Sol->GPIO%d\n", PIN_DHT22, PIN_SOIL_SENSOR);
    delay(2000);
    Serial.println("[SENSORS] Gata.");
}

void readAllSensors(SensorData &d) {
    d.timestamp = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    d.dhtValid    = !(isnan(t) || isnan(h));
    d.temperature = d.dhtValid ? t : 0.0f;
    d.humidity    = d.dhtValid ? h : 0.0f;
    if (!d.dhtValid) Serial.println("[SENSORS] ERR: DHT22!");

    int raw = analogRead(PIN_SOIL_SENSOR);
    d.soilValid = (raw >= 0 && raw <= 4095);
    d.soilRaw   = d.soilValid ? raw : 0;
    d.soilPct   = d.soilValid ? soilAdcToPercent(raw) : 0.0f;
    if (!d.soilValid) Serial.println("[SENSORS] ERR: Sol!");
}

float soilAdcToPercent(int adc) {
    if (adc >= SOIL_ADC_DRY) return 0.0f;
    if (adc <= SOIL_ADC_WET)  return 100.0f;
    return (float)(SOIL_ADC_DRY - adc) / (float)(SOIL_ADC_DRY - SOIL_ADC_WET) * 100.0f;
}

const char* getSoilStatus(float p) {
    if (p < 20) return "USCAT CRITIC";
    if (p < 40) return "USCAT";
    if (p < 60) return "OPTIM";
    if (p < 80) return "UMED";
    return "SATURAT";
}

void printSensorData(const SensorData &d) {
    Serial.println("┌──────────────────────────────┐");
    if (d.dhtValid) {
        Serial.printf("│ Temp:  %6.1f °C            │\n", d.temperature);
        Serial.printf("│ Umid:  %6.1f %%             │\n", d.humidity);
    } else Serial.println("│ Temp/Umid: EROARE           │");
    if (d.soilValid)
        Serial.printf("│ Sol:   %5.1f%% [%-12s] │\n", d.soilPct, getSoilStatus(d.soilPct));
    Serial.println("└──────────────────────────────┘");
}

String sensorDataToJson(const SensorData &d, const SystemState &s) {
    char buf[300];
    snprintf(buf, sizeof(buf),
        "{\"device\":\"%s\",\"temp\":%.1f,\"hum\":%.1f,"
        "\"soil_raw\":%d,\"soil_pct\":%.1f,"
        "\"windows\":%s,\"pump\":%s,\"fan1\":%s,\"fan2\":%s,"
        "\"auto\":%s,\"ts\":%lu}",
        DEVICE_ID, d.temperature, d.humidity,
        d.soilRaw, d.soilPct,
        s.windowsOpen?"true":"false", s.pumpRunning?"true":"false",
        s.fan1Running?"true":"false", s.fan2Running?"true":"false",
        s.autoMode?"true":"false", d.timestamp);
    return String(buf);
}
