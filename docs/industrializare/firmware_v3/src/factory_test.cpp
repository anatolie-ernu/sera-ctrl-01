/**
 * factory_test.cpp — End-of-line test implementation
 */
#include "factory_test.h"
#include "nvs_config.h"
#include "hardware.h"          // Production pin map (PCB SERA-CTRL-01)
#include <Adafruit_SHT31.h>
#include <WiFi.h>

extern Adafruit_SHT31 sht31;   // Defined in main.cpp

bool factoryTestRequested() {
    // Window of 5 s after boot in which the jig can send the magic string
    Serial.println("[FT] Send FACTORY_TEST within 5s to enter test mode");
    unsigned long t0 = millis();
    String rx;
    while (millis() - t0 < 5000) {
        while (Serial.available()) rx += (char)Serial.read();
        if (rx.indexOf("FACTORY_TEST") >= 0) return true;
        // BOOT button (GPIO0) held low also triggers test mode
        if (digitalRead(0) == LOW && millis() - t0 > 1000) return true;
        delay(10);
    }
    return false;
}

void runFactoryTest() {
    Serial.println("\n========== FACTORY TEST START ==========");
    bool pass = true;
    String fails = "";

    // ── 1. Relay click test ───────────────────────────────────────────────────
    const struct { uint8_t pin; const char* name; } relays[] = {
        {PIN_RELAY_WINDOW1, "K1-WIN1"}, {PIN_RELAY_WINDOW2, "K2-WIN2"},
        {PIN_RELAY_PUMP,    "K3-PUMP"}, {PIN_RELAY_FAN1,    "K4-FAN1"},
        {PIN_RELAY_FAN2,    "K5-FAN2"},
    };
    for (auto &r : relays) {
        Serial.printf("[FT] Relay %s ON\n", r.name);
        digitalWrite(r.pin, RELAY_ON);
        delay(500);                       // Jig measures output continuity here
        digitalWrite(r.pin, RELAY_OFF);
        delay(200);
    }

    // ── 2. SHT31 sensor test ─────────────────────────────────────────────────
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();
    bool shtOk = !isnan(t) && !isnan(h) && t > -10 && t < 50 && h > 5 && h < 100;
    Serial.printf("[FT] SHT31: T=%.1f H=%.1f → %s\n", t, h, shtOk ? "PASS" : "FAIL");
    if (!shtOk) { pass = false; fails += "SHT31;"; }

    // ── 3. Soil ADC test ─────────────────────────────────────────────────────
    // The jig fits a divider giving ~1.65 V → ADC mid-range (1500–2600 accepted)
    int adc = analogRead(PIN_SOIL_SENSOR);
    bool adcOk = adc > 1500 && adc < 2600;
    Serial.printf("[FT] Soil ADC: %d → %s\n", adc, adcOk ? "PASS" : "FAIL");
    if (!adcOk) { pass = false; fails += "SOIL_ADC;"; }

    // ── 4. WiFi / antenna test ───────────────────────────────────────────────
    WiFi.mode(WIFI_STA);
    int nets = WiFi.scanNetworks();
    bool wifiOk = nets > 0;
    Serial.printf("[FT] WiFi scan: %d networks → %s\n", nets, wifiOk ? "PASS" : "FAIL");
    if (!wifiOk) { pass = false; fails += "WIFI;"; }

    // ── 5. Machine-readable verdict (parsed by the test jig) ────────────────
    Serial.printf("TEST_RESULT;%s;%s;%s\n",
                  cfg.device_id, pass ? "PASS" : "FAIL",
                  pass ? "OK" : fails.c_str());
    Serial.println("========== FACTORY TEST END ==========");
    Serial.println("[FT] Power-cycle the unit to exit test mode.");
    while (true) delay(1000);   // Halt — jig powers the unit off
}
