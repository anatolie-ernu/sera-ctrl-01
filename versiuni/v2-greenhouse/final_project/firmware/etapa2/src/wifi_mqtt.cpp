/**
 * wifi_mqtt.cpp  —  Greenhouse IoT System | Firmware Stage 2
 * ─────────────────────────────────────────────────────────────────────────────
 * Implements all network communication for the ESP32:
 *   • WiFi station mode with configurable timeout and periodic retry
 *   • MQTT publish / subscribe using the PubSubClient library
 *   • JSON command parsing using ArduinoJson (stack-allocated documents)
 *   • NTP time synchronisation (configTzTime) for accurate timestamps
 *
 * MQTT command processing
 * ─────────────────────────────────────────────────────────────────────────────
 * When a message arrives on a subscribed topic, onCmd() is called by the
 * PubSubClient library from within mqtt.loop().  It parses the JSON payload,
 * performs the requested actuator action, and publishes a heartbeat so the
 * server immediately sees the updated state.
 *
 * Accepted payload formats (all topics):
 *   {"action":"OPEN"}
 *   {"action":"ON","duration_ms":60000}   ← pump with 60-second timer
 *   {"action":"ON","fan":1}               ← specific fan (0 = all)
 *   "ON" or "OPEN"                        ← plain string fallback
 *
 * Security note
 * ─────────────────────────────────────────────────────────────────────────────
 * This implementation uses plain MQTT (port 1883) which is acceptable on a
 * private LAN.  For internet-facing deployments, upgrade to MQTTS (port 8883)
 * with a TLS certificate — replace WiFiClient with WiFiClientSecure and set
 * the CA certificate with setCACert().
 */
#include "wifi_mqtt.h"
#include "actuators.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Externally defined in main.cpp — these are the live state structs
extern SystemState sS;
extern SensorData  sD;

// Network clients — static so they live for the duration of program execution
static WiFiClient   wc;
static PubSubClient mqtt(wc);

// Timestamps for the reconnect rate-limiters (avoid spamming reconnect attempts)
static unsigned long _lastWifiRetry = 0;
static unsigned long _lastMqttRetry = 0;

// ─────────────────────────────────────────────────────────────────────────────
// MQTT COMMAND CALLBACK
// Called by PubSubClient whenever a message arrives on any subscribed topic.
// Runs inside mqtt.loop() so keep it short; no delay() calls here.
// ─────────────────────────────────────────────────────────────────────────────
static void onCmd(char* topic, byte* payload, unsigned int len) {
    String t = String(topic);
    String m((char*)payload, len);
    Serial.printf("[MQTT←] %s → %s\n", topic, m.c_str());

    // Use a stack-allocated JSON document (no heap allocation, no memory leak)
    // 256 bytes is sufficient for all command payloads
    StaticJsonDocument<256> doc;

    // If JSON parse fails, treat the raw string as the action value
    if (deserializeJson(doc, m)) doc["action"] = m;
    const char* action = doc["action"] | "unknown";

    // ── Window commands ───────────────────────────────────────────────────────
    if (t == TOPIC_CMD_WIN) {
        if (!strcmp(action, "OPEN"))  { openWindows();  sS.windowsOpen = true; }
        else                          { closeWindows(); sS.windowsOpen = false; }
    }

    // ── Pump commands ─────────────────────────────────────────────────────────
    // duration_ms is optional; 0 means run until PUMP_OFF command
    else if (t == TOPIC_CMD_PUMP) {
        if (!strcmp(action, "ON")) {
            unsigned long dur = doc["duration_ms"] | (unsigned long)0;
            startPump(dur);
            sS.pumpRunning   = true;
            sS.pumpStartTime = millis();
            sS.pumpDuration  = dur;
        } else {
            stopPump();
            sS.pumpRunning = false;
        }
    }

    // ── Fan commands ──────────────────────────────────────────────────────────
    // "fan":0 (or absent) means all fans; "fan":1 or "fan":2 is individual
    else if (t == TOPIC_CMD_FAN) {
        int fn = doc["fan"] | 0;
        if (!strcmp(action, "ON")) {
            if (fn == 0) { startAllFans(); sS.fan1Running = sS.fan2Running = true; }
            else {
                startFan(fn);
                if (fn == 1) sS.fan1Running = true; else sS.fan2Running = true;
            }
        } else {
            if (fn == 0) { stopAllFans(); sS.fan1Running = sS.fan2Running = false; }
            else {
                stopFan(fn);
                if (fn == 1) sS.fan1Running = false; else sS.fan2Running = false;
            }
        }
    }

    // ── Autonomous mode toggle ────────────────────────────────────────────────
    else if (t == TOPIC_CMD_AUTO) {
        sS.autoMode = !strcmp(action, "ON");
        Serial.printf("[MQTT] Auto mode: %s\n", sS.autoMode ? "ON" : "OFF");
    }

    // Immediately report new state to server after executing any command
    publishHeartbeat(sS);
}

// ─────────────────────────────────────────────────────────────────────────────
// WiFi CONNECTION
// ─────────────────────────────────────────────────────────────────────────────
static bool connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.printf("[WiFi] Connecting to %s ", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Poll with a 500 ms cadence up to WIFI_TIMEOUT_S seconds
    int attempts = WIFI_TIMEOUT_S * 2;
    while (WiFi.status() != WL_CONNECTED && attempts-- > 0) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected! IP: %s  RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
        return true;
    }

    Serial.println("[WiFi] Connection failed — will retry later");
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// MQTT CONNECTION
// ─────────────────────────────────────────────────────────────────────────────
static bool connectMQTT() {
    if (mqtt.connected()) return true;
    if (WiFi.status() != WL_CONNECTED) return false;

    Serial.printf("[MQTT] Connecting to %s:%d ...\n", MQTT_HOST, MQTT_PORT);

    // Last Will Testament (LWT):
    // If the ESP32 loses power or crashes, the MQTT broker publishes this
    // retained message automatically on behalf of the device.  The server
    // subscribes to TOPIC_HEARTBEAT and can detect the device going offline.
    const char* lwtMsg  = "{\"online\":false}";
    const bool  retain  = true;
    const int   qos     = 1;

    bool ok = mqtt.connect(
        MQTT_CLIENT_ID,
        MQTT_USER, MQTT_PASS,
        TOPIC_HEARTBEAT, qos, retain, lwtMsg
    );

    if (ok) {
        Serial.println("[MQTT] Connected! Subscribing to command topics...");
        // Subscribe with QoS 1 so commands are re-delivered if the connection
        // drops momentarily while a command is in-flight
        mqtt.subscribe(TOPIC_CMD_WIN,  1);
        mqtt.subscribe(TOPIC_CMD_PUMP, 1);
        mqtt.subscribe(TOPIC_CMD_FAN,  1);
        mqtt.subscribe(TOPIC_CMD_AUTO, 1);
        mqtt.subscribe(TOPIC_CMD_CFG,  1);
        return true;
    }

    Serial.printf("[MQTT] Failed — state: %d  (see PubSubClient.h for codes)\n",
                  mqtt.state());
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
void initWiFiMQTT() {
    // Configure the POSIX timezone so localtime() and strftime() work correctly.
    // The string "EET-2EEST,M3.5.0/3,M10.5.0/4" encodes the Romanian DST rules.
    configTzTime(NTP_TZ, NTP_SERVER);

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onCmd);
    mqtt.setKeepAlive(MQTT_KEEPALIVE);
    mqtt.setBufferSize(512);  // Increase from 256-byte default for sensor payloads

    if (connectWiFi()) {
        // Wait up to 5 s for NTP sync before continuing
        Serial.print("[NTP] Syncing");
        time_t now = 0;
        int retries = 10;
        while (now < 1000000000L && retries-- > 0) {
            time(&now);
            delay(500);
            Serial.print('.');
        }
        Serial.println(now > 1000000000L ? " OK" : " TIMEOUT (using millis)");
        connectMQTT();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void mqttLoop() {
    unsigned long now = millis();

    // Rate-limited WiFi reconnect check (every WIFI_RETRY_MS)
    if (now - _lastWifiRetry >= WIFI_RETRY_MS) {
        _lastWifiRetry = now;
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Reconnecting...");
            connectWiFi();
        }
    }

    // Rate-limited MQTT reconnect check (every MQTT_RETRY_MS)
    if (now - _lastMqttRetry >= MQTT_RETRY_MS) {
        _lastMqttRetry = now;
        if (!mqtt.connected()) {
            Serial.println("[MQTT] Reconnecting...");
            connectMQTT();
        }
    }

    // Process incoming MQTT messages and send keepalive ping if due.
    // PubSubClient.loop() must be called frequently (much less than KEEPALIVE s).
    mqtt.loop();
}

// ─────────────────────────────────────────────────────────────────────────────
// PUBLISH: full sensor reading
// ─────────────────────────────────────────────────────────────────────────────
void publishSensorData(const SensorData &d, const SystemState &s) {
    if (!mqtt.connected()) return;

    // Get current Unix timestamp (seconds since epoch) for the "ts" field.
    // Falls back to a small number if NTP hasn't synced yet; the backend
    // will replace it with server time in that case.
    time_t now; time(&now);

    char buf[420];
    snprintf(buf, sizeof(buf),
        "{"
        "\"device\":\"%s\","
        "\"ts\":%ld,"
        "\"temp\":%.1f,\"hum\":%.1f,"
        "\"soil_raw\":%d,\"soil_pct\":%.1f,"
        "\"dht_ok\":%s,\"soil_ok\":%s,"
        "\"state\":{"
            "\"windows\":%s,\"pump\":%s,"
            "\"fan1\":%s,\"fan2\":%s,"
            "\"auto\":%s,"
            "\"wifi_rssi\":%d,"
            "\"heap_free\":%u"
        "}"
        "}",
        DEVICE_ID, (long)now,
        d.temperature, d.humidity,
        d.soilRaw, d.soilPct,
        d.dhtValid  ? "true" : "false",
        d.soilValid ? "true" : "false",
        s.windowsOpen  ? "true" : "false",
        s.pumpRunning  ? "true" : "false",
        s.fan1Running  ? "true" : "false",
        s.fan2Running  ? "true" : "false",
        s.autoMode     ? "true" : "false",
        WiFi.RSSI(),
        (unsigned int)ESP.getFreeHeap()   // Useful for detecting memory leaks
    );

    // retained=true: the broker stores the last known sensor state.
    // A client that connects after this publish can immediately read
    // the latest value without waiting for the next 30-second cycle.
    bool ok = mqtt.publish(TOPIC_SENSORS, buf, /* retained */ true);
    Serial.printf("[MQTT→] sensors: %s\n", ok ? "OK" : "FAIL");
}

// ─────────────────────────────────────────────────────────────────────────────
void publishAlert(const char* type, const char* msg) {
    if (!mqtt.connected()) return;
    time_t now; time(&now);

    char buf[220];
    snprintf(buf, sizeof(buf),
        "{\"device\":\"%s\",\"ts\":%ld,\"type\":\"%s\",\"message\":\"%s\"}",
        DEVICE_ID, (long)now, type, msg);

    // Alerts are NOT retained — we want each alert to be a new event,
    // not a persistent "last alert" shown to newly-connecting clients
    mqtt.publish(TOPIC_ALERTS, buf, /* retained */ false);
    Serial.printf("[MQTT→] alert [%s]: %s\n", type, msg);
}

// ─────────────────────────────────────────────────────────────────────────────
void publishHeartbeat(const SystemState &s) {
    if (!mqtt.connected()) return;
    time_t now; time(&now);

    char buf[220];
    snprintf(buf, sizeof(buf),
        "{\"device\":\"%s\",\"ts\":%ld,"
        "\"online\":true,"
        "\"windows\":%s,\"pump\":%s,"
        "\"auto\":%s,\"rssi\":%d}",
        DEVICE_ID, (long)now,
        s.windowsOpen ? "true" : "false",
        s.pumpRunning ? "true" : "false",
        s.autoMode    ? "true" : "false",
        WiFi.RSSI());

    // Retained=true: server always has the latest heartbeat even if it
    // restarts between heartbeat intervals
    mqtt.publish(TOPIC_HEARTBEAT, buf, /* retained */ true);
}

// ─────────────────────────────────────────────────────────────────────────────
bool isMqttConnected() { return mqtt.connected(); }
