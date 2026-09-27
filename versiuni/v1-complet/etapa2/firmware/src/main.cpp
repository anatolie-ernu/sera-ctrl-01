#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "actuators.h"
#include "wifi_mqtt.h"

SensorData  sD = {};
SystemState sS = {false,false,false,false,true,false,false,false,0,0};
unsigned long lastRead=0, lastHB=0;

void setup() {
    Serial.begin(115200); delay(500);
    Serial.println("\n╔══════════════════════════════╗");
    Serial.println(  "║  SERA INTELIGENTA v2.0       ║");
    Serial.printf(   "║  Device: %-20s║\n", DEVICE_ID);
    Serial.println(  "╚══════════════════════════════╝");
    initActuators(); initSensors(); initWiFiMQTT();
    sS.wifiOk=(WiFi.status()==WL_CONNECTED);
    sS.mqttOk=isMqttConnected();
    Serial.println("[MAIN] v2 gata. WiFi+MQTT activ.");
}

void loop() {
    unsigned long now=millis();
    mqttLoop();
    if(now-lastRead>=SENSOR_INTERVAL_MS) {
        lastRead=now;
        readAllSensors(sD); printSensorData(sD);
        sS.wifiOk=(WiFi.status()==WL_CONNECTED);
        sS.mqttOk=isMqttConnected();
        publishSensorData(sD,sS);
        sS.alertActive=false;
        // Alerte critice via MQTT
        if(sD.dhtValid && sD.temperature>=TEMP_ALERT_HIGH) {
            char m[80]; snprintf(m,sizeof(m),"T critica: %.1f C",sD.temperature);
            publishAlert("TEMP_HIGH",m);
        }
        if(sD.soilValid && sD.soilPct<SOIL_ALERT_DRY_PCT) {
            char m[80]; snprintf(m,sizeof(m),"Sol uscat: %.1f%%",sD.soilPct);
            publishAlert("SOIL_DRY",m);
        }
    }
    if(now-lastHB>=HEARTBEAT_INTERVAL_MS){lastHB=now;publishHeartbeat(sS);}
    if(sS.autoMode) applyThresholdLogic(sD,sS);
    checkPumpTimeout(sS);
    handleSerialCommands(sS);
    delay(100);
}
