#pragma once
#include <Arduino.h>
#include "config.h"
void initWiFiMQTT();
void mqttLoop();
void publishSensorData(const SensorData &d, const SystemState &s);
void publishAlert(const char* type, const char* msg);
void publishHeartbeat(const SystemState &s);
bool isMqttConnected();
