#pragma once
#include "config.h"
void        initSensors();
void        readAllSensors(SensorData &d);
void        printSensorData(const SensorData &d);
float       soilAdcToPercent(int adc);
const char* getSoilStatus(float pct);
String      sensorDataToJson(const SensorData &d, const SystemState &s);
