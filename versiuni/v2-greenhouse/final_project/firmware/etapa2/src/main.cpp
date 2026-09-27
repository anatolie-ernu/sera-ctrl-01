/**
 * main.cpp  —  Greenhouse IoT System | Firmware Stage 2
 * ─────────────────────────────────────────────────────────────────────────────
 * Extends Stage 1 with:
 *   • WiFi connection and automatic reconnection
 *   • MQTT publish (sensors, alerts, heartbeat) and subscribe (commands)
 *   • NTP time synchronisation
 *
 * The greenhouse threshold logic from Stage 1 continues to run every loop,
 * ensuring autonomous operation even when the network is unavailable.
 */
#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "actuators.h"
#include "wifi_mqtt.h"

// Global state — see Stage 1 main.cpp for detailed comments on these structs
SensorData  sD = {0, 0, 0, 0.0f, false, false, 0};
SystemState sS = {false,false,false,false,true,false,false,false,0,0};

unsigned long lastSensorRead = 0;   // Timestamp of last sensor poll
unsigned long lastHeartbeat  = 0;   // Timestamp of last MQTT heartbeat

void setup() {
    Serial.begin(115200); delay(500);
    Serial.println("\n╔══════════════════════════════╗");
    Serial.println(  "║  GREENHOUSE IoT v2.0         ║");
    Serial.printf(   "║  Device:  %-20s║\n", DEVICE_ID);
    Serial.println(  "╚══════════════════════════════╝");

    initActuators();   // Stage 1: relay GPIO setup
    initSensors();     // Stage 1: DHT22 + soil ADC setup
    initWiFiMQTT();    // Stage 2: WiFi connect + NTP + MQTT connect

    sS.wifiOk = (WiFi.status() == WL_CONNECTED);
    sS.mqttOk = isMqttConnected();

    Serial.println("[MAIN] v2 ready — WiFi + MQTT active");
}

void loop() {
    unsigned long now = millis();

    // ── MQTT maintenance (must run every loop) ────────────────────────────────
    // Processes incoming command messages and sends MQTT keepalive pings.
    // Also checks and re-establishes WiFi and MQTT connections if needed.
    mqttLoop();

    // ── Sensor polling ────────────────────────────────────────────────────────
    if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
        lastSensorRead = now;
        readAllSensors(sD);
        printSensorData(sD);

        // Update connectivity state for the heartbeat payload
        sS.wifiOk = (WiFi.status() == WL_CONNECTED);
        sS.mqttOk = isMqttConnected();

        // Publish sensor data to the MQTT broker for the backend to store
        publishSensorData(sD, sS);

        // Publish MQTT alerts for critical conditions
        if (sD.dhtValid && sD.temperature >= TEMP_ALERT_HIGH) {
            char m[80];
            snprintf(m, sizeof(m), "Critical temperature: %.1f C", sD.temperature);
            publishAlert("TEMP_HIGH", m);
        }
        if (sD.soilValid && sD.soilPct < SOIL_ALERT_DRY_PCT) {
            char m[80];
            snprintf(m, sizeof(m), "Soil too dry: %.1f%%", sD.soilPct);
            publishAlert("SOIL_DRY", m);
        }
        sS.alertActive = false;
    }

    // ── Periodic heartbeat ────────────────────────────────────────────────────
    // Lighter than a full sensor publish; lets the server know the device
    // is still alive between the 30-second sensor publish intervals.
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = now;
        publishHeartbeat(sS);
    }

    // ── Autonomous threshold logic (Stage 1 behaviour, always active) ─────────
    if (sS.autoMode) applyThresholdLogic(sD, sS);

    // ── Timed pump watchdog ───────────────────────────────────────────────────
    checkPumpTimeout(sS);

    // ── Serial debug interface ────────────────────────────────────────────────
    handleSerialCommands(sS);

    delay(100);
}
