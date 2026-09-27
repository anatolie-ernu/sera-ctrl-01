/**
 * wifi_mqtt.h  —  Greenhouse IoT System | Firmware Stage 2
 * ─────────────────────────────────────────────────────────────────────────────
 * Public interface for WiFi management, MQTT client, and NTP time sync.
 *
 * Key design decisions
 * ─────────────────────────────────────────────────────────────────────────────
 * RESILIENCE: WiFi and MQTT reconnect logic runs inside mqttLoop() which is
 * called every loop() iteration.  The greenhouse threshold logic continues
 * running whether or not the network is available.
 *
 * LWT (Last Will Testament): when the MQTT connection is established, the
 * broker is instructed to publish {"online":false} to the heartbeat topic if
 * the ESP32 disconnects ungracefully.  This lets the server detect device
 * failure without waiting for a heartbeat timeout.
 *
 * NTP: on first successful WiFi connection, time is synchronised from
 * pool.ntp.org with the Europe/Bucharest timezone.  All MQTT payloads include
 * a Unix timestamp (ts field) so stored readings have correct wall-clock time.
 *
 * Implemented in: wifi_mqtt.cpp
 * Depends on:     config.h, WiFi.h, PubSubClient, ArduinoJson
 */
#pragma once
#include <Arduino.h>
#include "config.h"

/**
 * Connect to WiFi, sync NTP time, and connect to MQTT broker.
 * Subscribes to all command topics on successful connection.
 * Call once in setup() after initActuators() and initSensors().
 */
void initWiFiMQTT();

/**
 * Must be called every loop() iteration.
 * Handles: MQTT packet processing, keepalive pings, automatic
 * reconnection for both WiFi and MQTT if they drop.
 */
void mqttLoop();

/**
 * Publish a full sensor reading + relay state as a JSON payload to
 * TOPIC_SENSORS with retained=true.  No-op if MQTT is not connected.
 */
void publishSensorData(const SensorData &d, const SystemState &s);

/**
 * Publish an alert event (e.g. critical temperature) to TOPIC_ALERTS.
 * @param type    Short identifier: "TEMP_HIGH", "TEMP_LOW", "SOIL_DRY"
 * @param msg     Human-readable description of the condition
 */
void publishAlert(const char* type, const char* msg);

/**
 * Publish the current relay state + WiFi RSSI to TOPIC_HEARTBEAT.
 * Called every HEARTBEAT_INTERVAL_MS and after every command execution.
 */
void publishHeartbeat(const SystemState &s);

/** @return true if the MQTT broker connection is currently active */
bool isMqttConnected();
