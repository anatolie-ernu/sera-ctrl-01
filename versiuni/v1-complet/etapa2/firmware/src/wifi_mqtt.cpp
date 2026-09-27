/**
 * wifi_mqtt.cpp — WiFi + MQTT + NTP pentru ESP32
 * - Conectare WiFi cu retry automat
 * - MQTT PubSubClient: publish date, subscribe comenzi JSON
 * - LWT (Last Will Testament) — server detecteaza caderea offline
 * - NTP time sync pentru timestamps corecte
 */
#include "wifi_mqtt.h"
#include "actuators.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

extern SystemState sS;
extern SensorData  sD;

static WiFiClient   wc;
static PubSubClient mqtt(wc);
static unsigned long _lwifi=0, _lmqtt=0;

// ── Callback comenzi primite ─────────────────────────────────
static void onCmd(char* topic, byte* pay, unsigned int len) {
    String t=String(topic), m((char*)pay,len);
    Serial.printf("[MQTT<] %s → %s\n",topic,m.c_str());
    StaticJsonDocument<256> doc;
    if(deserializeJson(doc,m)) doc["action"]=m;
    const char* a=doc["action"]|"?";

    if(t==TOPIC_CMD_WIN) {
        if(!strcmp(a,"OPEN")){openWindows();sS.windowsOpen=true;}
        else{closeWindows();sS.windowsOpen=false;}
    }
    else if(t==TOPIC_CMD_PUMP) {
        if(!strcmp(a,"ON")){
            unsigned long dur=doc["duration_ms"]|0UL;
            startPump(dur);sS.pumpRunning=true;
            sS.pumpStartTime=millis();sS.pumpDuration=dur;
        } else {stopPump();sS.pumpRunning=false;}
    }
    else if(t==TOPIC_CMD_FAN) {
        int fn=doc["fan"]|0;
        if(!strcmp(a,"ON")){
            if(fn==0){startAllFans();sS.fan1Running=sS.fan2Running=true;}
            else{startFan(fn);if(fn==1)sS.fan1Running=true;else sS.fan2Running=true;}
        } else {
            if(fn==0){stopAllFans();sS.fan1Running=sS.fan2Running=false;}
            else{stopFan(fn);if(fn==1)sS.fan1Running=false;else sS.fan2Running=false;}
        }
    }
    else if(t==TOPIC_CMD_AUTO) sS.autoMode=(!strcmp(a,"ON"));
    publishHeartbeat(sS);
}

static bool connectWiFi() {
    if(WiFi.status()==WL_CONNECTED) return true;
    Serial.printf("[WiFi] Conectare %s",WIFI_SSID);
    WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
    int n=WIFI_TIMEOUT_S*2;
    while(WiFi.status()!=WL_CONNECTED && n-->0){delay(500);Serial.print(".");}
    Serial.println();
    if(WiFi.status()==WL_CONNECTED){
        Serial.printf("[WiFi] IP:%s RSSI:%d\n",WiFi.localIP().toString().c_str(),WiFi.RSSI());
        return true;
    }
    Serial.println("[WiFi] FAIL"); return false;
}

static bool connectMQTT() {
    if(mqtt.connected()) return true;
    if(WiFi.status()!=WL_CONNECTED) return false;
    Serial.printf("[MQTT] Conectare %s:%d\n",MQTT_HOST,MQTT_PORT);
    // LWT — publicat automat de broker daca ESP32 cade
    bool ok=mqtt.connect(MQTT_CLIENT_ID,MQTT_USER,MQTT_PASS,
        TOPIC_HEARTBEAT,1,true,"{\"online\":false}");
    if(ok){
        Serial.println("[MQTT] Conectat! Abonare topicuri...");
        mqtt.subscribe(TOPIC_CMD_WIN);  mqtt.subscribe(TOPIC_CMD_PUMP);
        mqtt.subscribe(TOPIC_CMD_FAN);  mqtt.subscribe(TOPIC_CMD_AUTO);
        mqtt.subscribe(TOPIC_CMD_CFG);
    } else Serial.printf("[MQTT] FAIL cod:%d\n",mqtt.state());
    return ok;
}

void initWiFiMQTT() {
    configTzTime(NTP_TZ, NTP_SERVER);
    mqtt.setServer(MQTT_HOST,MQTT_PORT);
    mqtt.setCallback(onCmd);
    mqtt.setKeepAlive(MQTT_KEEPALIVE);
    mqtt.setBufferSize(512);
    if(connectWiFi()){
        Serial.print("[NTP] Sync"); time_t now=0; int r=10;
        while(now<1000000000L && r-->0){time(&now);delay(500);Serial.print(".");}
        Serial.println(now>1000000000L?" OK":" FAIL");
        connectMQTT();
    }
}

void mqttLoop() {
    unsigned long now=millis();
    if(now-_lwifi>=WIFI_RETRY_MS){_lwifi=now;
        if(WiFi.status()!=WL_CONNECTED){Serial.println("[WiFi] Reconectare...");connectWiFi();}}
    if(now-_lmqtt>=MQTT_RETRY_MS){_lmqtt=now;
        if(!mqtt.connected()){Serial.println("[MQTT] Reconectare...");connectMQTT();}}
    mqtt.loop();
}

void publishSensorData(const SensorData &d, const SystemState &s) {
    if(!mqtt.connected()) return;
    time_t now; time(&now);
    char buf[400];
    snprintf(buf,sizeof(buf),
        "{\"device\":\"%s\",\"ts\":%ld,"
        "\"temp\":%.1f,\"hum\":%.1f,\"soil_raw\":%d,\"soil_pct\":%.1f,"
        "\"dht_ok\":%s,\"soil_ok\":%s,"
        "\"state\":{\"windows\":%s,\"pump\":%s,\"fan1\":%s,\"fan2\":%s,"
        "\"auto\":%s,\"wifi_rssi\":%d,\"heap\":%u}}",
        DEVICE_ID,(long)now,
        d.temperature,d.humidity,d.soilRaw,d.soilPct,
        d.dhtValid?"true":"false",d.soilValid?"true":"false",
        s.windowsOpen?"true":"false",s.pumpRunning?"true":"false",
        s.fan1Running?"true":"false",s.fan2Running?"true":"false",
        s.autoMode?"true":"false",WiFi.RSSI(),(unsigned)ESP.getFreeHeap());
    bool ok=mqtt.publish(TOPIC_SENSORS,buf,true);
    Serial.printf("[MQTT>] senzori: %s\n",ok?"OK":"FAIL");
}

void publishAlert(const char* type, const char* msg) {
    if(!mqtt.connected()) return;
    time_t now; time(&now);
    char buf[200];
    snprintf(buf,sizeof(buf),"{\"device\":\"%s\",\"ts\":%ld,\"type\":\"%s\",\"message\":\"%s\"}",
        DEVICE_ID,(long)now,type,msg);
    mqtt.publish(TOPIC_ALERTS,buf,false);
    Serial.printf("[MQTT>] alerta: %s - %s\n",type,msg);
}

void publishHeartbeat(const SystemState &s) {
    if(!mqtt.connected()) return;
    time_t now; time(&now);
    char buf[200];
    snprintf(buf,sizeof(buf),
        "{\"device\":\"%s\",\"ts\":%ld,\"online\":true,"
        "\"windows\":%s,\"pump\":%s,\"auto\":%s,\"rssi\":%d}",
        DEVICE_ID,(long)now,
        s.windowsOpen?"true":"false",s.pumpRunning?"true":"false",
        s.autoMode?"true":"false",WiFi.RSSI());
    mqtt.publish(TOPIC_HEARTBEAT,buf,true);
}

bool isMqttConnected(){ return mqtt.connected(); }
