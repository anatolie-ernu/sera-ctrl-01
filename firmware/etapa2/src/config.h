/**
 * config.h — Sera Inteligenta Etapa 2
 * Adaugat: WiFi + MQTT + NTP fata de Etapa 1
 */
#pragma once

#define FIRMWARE_VERSION   "2.0.0"
#define DEVICE_ID          "SERA_001"

// ── WiFi ──────────────────────────────────────────────────────
#define WIFI_SSID          "NumeleReteleiTale"
#define WIFI_PASSWORD      "ParolaWiFi"
#define WIFI_TIMEOUT_S     20
#define WIFI_RETRY_MS      30000UL

// ── MQTT ──────────────────────────────────────────────────────
#define MQTT_HOST          "192.168.1.100"   // IP server Linux
#define MQTT_PORT          1883
#define MQTT_USER          "sera_device"
#define MQTT_PASS          "schimba_parola_mqtt"
#define MQTT_CLIENT_ID     DEVICE_ID
#define MQTT_KEEPALIVE     60
#define MQTT_RETRY_MS      10000UL

// Topics PUBLISH  (ESP32 → Server)
#define TOPIC_SENSORS    "greenhouse/" DEVICE_ID "/sensors"
#define TOPIC_ALERTS     "greenhouse/" DEVICE_ID "/alerts"
#define TOPIC_HEARTBEAT  "greenhouse/" DEVICE_ID "/heartbeat"

// Topics SUBSCRIBE (Server → ESP32)
#define TOPIC_CMD_WIN    "greenhouse/" DEVICE_ID "/cmd/windows"
#define TOPIC_CMD_PUMP   "greenhouse/" DEVICE_ID "/cmd/pump"
#define TOPIC_CMD_FAN    "greenhouse/" DEVICE_ID "/cmd/fan"
#define TOPIC_CMD_AUTO   "greenhouse/" DEVICE_ID "/cmd/auto"
#define TOPIC_CMD_CFG    "greenhouse/" DEVICE_ID "/cmd/config"

// NTP
#define NTP_SERVER       "pool.ntp.org"
#define NTP_TZ           "EET-2EEST,M3.5.0/3,M10.5.0/4"  // Romania

// ── Pini (identici cu Etapa 1) ────────────────────────────────
#define PIN_DHT22          4
#define PIN_SOIL_SENSOR    34
#define PIN_RELAY_WINDOW1  16
#define PIN_RELAY_WINDOW2  17
#define PIN_RELAY_PUMP     18
#define PIN_RELAY_FAN1     19
#define PIN_RELAY_FAN2     21
#define PIN_LED_STATUS     2
#define DHT_TYPE           DHT22

// ── Intervale ─────────────────────────────────────────────────
#define SENSOR_INTERVAL_MS    30000UL
#define HEARTBEAT_INTERVAL_MS 60000UL
#define RELAY_DEBOUNCE_MS     2000UL
#define WINDOW_TRAVEL_TIME_MS 15000UL

// ── Praguri ───────────────────────────────────────────────────
#define TEMP_OPEN_WINDOWS    28.0f
#define TEMP_CLOSE_WINDOWS   24.0f
#define TEMP_START_FANS      26.0f
#define TEMP_STOP_FANS       23.0f
#define TEMP_ALERT_HIGH      35.0f
#define TEMP_ALERT_LOW        5.0f
#define HUMIDITY_ALERT_HIGH  90.0f
#define HUMIDITY_ALERT_LOW   30.0f
#define SOIL_ADC_DRY         3200
#define SOIL_ADC_WET          800
#define SOIL_ALERT_DRY_PCT   20.0f

#define RELAY_ACTIVE_LOW     true
#define RELAY_ON             (RELAY_ACTIVE_LOW ? LOW  : HIGH)
#define RELAY_OFF            (RELAY_ACTIVE_LOW ? HIGH : LOW)

struct SensorData {
    float temperature, humidity;
    int   soilRaw;
    float soilPct;
    bool  dhtValid, soilValid;
    unsigned long timestamp;
};

struct SystemState {
    bool windowsOpen, pumpRunning, fan1Running, fan2Running;
    bool autoMode, alertActive, wifiOk, mqttOk;
    unsigned long pumpStartTime, pumpDuration;
};
