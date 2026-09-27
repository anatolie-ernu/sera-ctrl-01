/**
 * main.cpp — Sera Inteligenta v3.0 | Firmware PRODUCTIE
 * ─────────────────────────────────────────────────────────────────────────────
 * Production boot sequence:
 *
 *   power-on
 *     │
 *     ├─→ factoryTestRequested()?  → runFactoryTest()        [end-of-line jig]
 *     │
 *     ├─→ cfgLoad()                                          [NVS config]
 *     │
 *     ├─→ provisioningNeeded()?    → runProvisioningPortal() [customer setup]
 *     │
 *     └─→ normal operation:
 *           sensors (SHT31 + soil) → MQTT publish
 *           threshold logic (autonomous, works offline)
 *           OTA listener on cmd/ota
 *           config updates on cmd/config (persisted to NVS)
 *           hardware watchdog (45 s) — resets the MCU if loop stalls
 *
 * BOOT button behaviour at runtime:
 *   hold 5 s  → re-enter provisioning portal (change WiFi)
 *   hold 10 s → factory reset (wipe NVS, reboot)
 */
#include <Arduino.h>
#include <Wire.h>
#include <esp_task_wdt.h>
#include <Adafruit_SHT31.h>
#include "hardware.h"
#include "nvs_config.h"
#include "provisioning.h"
#include "ota_update.h"
#include "factory_test.h"
// NOTE: sensors/actuators/wifi_mqtt modules from Stage 2 are reused with the
// hardware.h pin map and cfg.* thresholds replacing the old #define constants.

Adafruit_SHT31 sht31;

unsigned long lastSensorRead = 0;
unsigned long lastHeartbeat  = 0;
unsigned long btnPressStart  = 0;
bool          otaValidated   = false;

#define SENSOR_INTERVAL_MS    30000UL
#define HEARTBEAT_INTERVAL_MS 60000UL
#define WDT_TIMEOUT_S         45      // Watchdog: reset if loop stalls 45 s

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BTN_BOOT, INPUT_PULLUP);
    pinMode(PIN_LED_POWER,  OUTPUT); digitalWrite(PIN_LED_POWER, HIGH);
    pinMode(PIN_LED_WIFI,   OUTPUT);
    pinMode(PIN_LED_STATUS, OUTPUT);

    Serial.printf("\n=== SERA v%d.%d.%d | %s ===\n",
        FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR,
        FIRMWARE_VERSION_PATCH, HW_REVISION);

    // ── 1. Factory test gate (production line only) ──────────────────────────
    if (factoryTestRequested()) {
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
        sht31.begin(SHT31_ADDR);
        runFactoryTest();   // Never returns
    }

    // ── 2. Load persistent configuration ─────────────────────────────────────
    cfgLoad();

    // ── 3. Provisioning gate (customer first-time setup) ─────────────────────
    if (provisioningNeeded()) {
        runProvisioningPortal();   // Reboots after save — never returns
    }

    // ── 4. Hardware init ──────────────────────────────────────────────────────
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (!sht31.begin(SHT31_ADDR)) {
        Serial.println("[MAIN] WARNING: SHT31 not responding!");
    }
    // initActuators(); initWiFiMQTT();  ← Stage-2 modules, using cfg.* values

    // ── 5. Hardware watchdog ──────────────────────────────────────────────────
    // If loop() fails to call esp_task_wdt_reset() within WDT_TIMEOUT_S,
    // the MCU resets automatically.  In the field this recovers the device
    // from any firmware deadlock without a site visit.
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    Serial.printf("[MAIN] %s \"%s\" ready (boot #%u)\n",
                  cfg.device_id, cfg.device_name, cfg.boot_count);
}

// ─────────────────────────────────────────────────────────────────────────────
/** BOOT button runtime handler: 5 s → provisioning, 10 s → factory reset. */
static void handleButton() {
    if (digitalRead(PIN_BTN_BOOT) == LOW) {
        if (btnPressStart == 0) btnPressStart = millis();
        unsigned long held = millis() - btnPressStart;
        // Blink WiFi LED as feedback while holding
        digitalWrite(PIN_LED_WIFI, (held / 250) % 2);
        if (held >= 10000) cfgFactoryReset();          // Wipe + reboot
    } else {
        if (btnPressStart != 0) {
            unsigned long held = millis() - btnPressStart;
            btnPressStart = 0;
            if (held >= 5000) {                        // 5–10 s → re-provision
                cfg.provisioned = false;
                cfgSave();
                ESP.restart();
            }
        }
    }
}

void loop() {
    esp_task_wdt_reset();   // Feed the watchdog — FIRST thing every loop
    handleButton();

    unsigned long now = millis();

    // mqttLoop();          ← Stage-2 module: reconnect + process commands

    // ── Mark OTA image valid after first successful MQTT connect ─────────────
    // Until this runs, a reset would roll back to the previous firmware.
    // if (!otaValidated && isMqttConnected()) {
    //     otaMarkValid();
    //     otaValidated = true;
    // }

    if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
        lastSensorRead = now;
        float t = sht31.readTemperature();
        float h = sht31.readHumidity();
        int soil = analogRead(PIN_SOIL_SENSOR);
        Serial.printf("[SENS] T=%.1f H=%.1f soil=%d\n", t, h, soil);
        // publishSensorData(...) + applyThresholdLogic(...) — Stage-2 modules
        // using cfg.temp_open_windows etc. instead of compile-time constants
    }

    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = now;
        // publishHeartbeat(...);
    }

    delay(50);
}
