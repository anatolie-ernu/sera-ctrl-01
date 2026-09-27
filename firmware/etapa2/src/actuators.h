#pragma once
#include "config.h"
void initActuators();
void openWindows(); void closeWindows();
void startPump(unsigned long durMs = 0); void stopPump();
void startFan(uint8_t n); void stopFan(uint8_t n);
void startAllFans(); void stopAllFans();
void emergencyStop();
void applyThresholdLogic(const SensorData &d, SystemState &s);
void checkPumpTimeout(SystemState &s);
void handleSerialCommands(SystemState &s);
