/**
 * main.cpp — Sera Inteligenta Etapa 1
 * Senzori + Relee + Logica locala (fara WiFi)
 */
#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "actuators.h"

SensorData  sD = {};
SystemState sS = {false,false,false,false,true,false,0,0};
unsigned long lastRead = 0;

void setup() {
    Serial.begin(115200); delay(500);
    Serial.println("\n╔══════════════════════════════╗");
    Serial.println(  "║  SERA INTELIGENTA v1.0       ║");
    Serial.printf(   "║  Device: %-20s║\n", DEVICE_ID);
    Serial.println(  "╚══════════════════════════════╝");
    initActuators();
    initSensors();
    Serial.println("[MAIN] Gata. Auto=ON. Scrie HELP.");
}

void loop() {
    unsigned long now = millis();
    if (now - lastRead >= SENSOR_INTERVAL_MS) {
        lastRead = now;
        readAllSensors(sD);
        printSensorData(sD);
        sS.alertActive = false;
    }
    if (sS.autoMode) applyThresholdLogic(sD, sS);
    checkPumpTimeout(sS);
    handleSerialCommands(sS);
    delay(100);
}
