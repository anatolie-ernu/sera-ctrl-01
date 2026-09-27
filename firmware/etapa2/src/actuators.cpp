#include "actuators.h"
#include <Arduino.h>

static unsigned long _lwc=0, _lpc=0, _lfc=0;

void initActuators() {
    uint8_t pins[]={PIN_RELAY_WINDOW1,PIN_RELAY_WINDOW2,PIN_RELAY_PUMP,
                    PIN_RELAY_FAN1,PIN_RELAY_FAN2,PIN_LED_STATUS};
    for(auto p:pins) pinMode(p,OUTPUT);
    digitalWrite(PIN_RELAY_WINDOW1,RELAY_OFF); digitalWrite(PIN_RELAY_WINDOW2,RELAY_OFF);
    digitalWrite(PIN_RELAY_PUMP,RELAY_OFF);    digitalWrite(PIN_RELAY_FAN1,RELAY_OFF);
    digitalWrite(PIN_RELAY_FAN2,RELAY_OFF);    digitalWrite(PIN_LED_STATUS,LOW);
    Serial.println("[ACT] Relee init - toate OFF");
}

void openWindows() {
    if(millis()-_lwc<RELAY_DEBOUNCE_MS) return;
    Serial.println("[WIN] DESCHIDERE");
    digitalWrite(PIN_RELAY_WINDOW1,RELAY_ON); digitalWrite(PIN_RELAY_WINDOW2,RELAY_ON);
    delay(WINDOW_TRAVEL_TIME_MS);
    digitalWrite(PIN_RELAY_WINDOW1,RELAY_OFF); digitalWrite(PIN_RELAY_WINDOW2,RELAY_OFF);
    _lwc=millis(); Serial.println("[WIN] Deschise.");
}

void closeWindows() {
    if(millis()-_lwc<RELAY_DEBOUNCE_MS) return;
    Serial.println("[WIN] INCHIDERE");
    digitalWrite(PIN_RELAY_WINDOW1,RELAY_ON); digitalWrite(PIN_RELAY_WINDOW2,RELAY_ON);
    delay(WINDOW_TRAVEL_TIME_MS);
    digitalWrite(PIN_RELAY_WINDOW1,RELAY_OFF); digitalWrite(PIN_RELAY_WINDOW2,RELAY_OFF);
    _lwc=millis(); Serial.println("[WIN] Inchise.");
}

void startPump(unsigned long durMs) {
    if(millis()-_lpc<RELAY_DEBOUNCE_MS) return;
    Serial.printf("[PUMP] ON (dur:%lums)\n",durMs);
    digitalWrite(PIN_RELAY_PUMP,RELAY_ON); digitalWrite(PIN_LED_STATUS,HIGH);
    _lpc=millis();
}

void stopPump() {
    if(millis()-_lpc<RELAY_DEBOUNCE_MS) return;
    Serial.println("[PUMP] OFF");
    digitalWrite(PIN_RELAY_PUMP,RELAY_OFF); digitalWrite(PIN_LED_STATUS,LOW);
    _lpc=millis();
}

void checkPumpTimeout(SystemState &s) {
    if(s.pumpRunning && s.pumpDuration>0 && millis()-s.pumpStartTime>=s.pumpDuration) {
        Serial.println("[PUMP] Timer expirat - auto stop");
        stopPump(); s.pumpRunning=false; s.pumpDuration=0;
    }
}

void startFan(uint8_t n) {
    if(millis()-_lfc<RELAY_DEBOUNCE_MS) return;
    if(n==1){Serial.println("[FAN1] ON"); digitalWrite(PIN_RELAY_FAN1,RELAY_ON);}
    if(n==2){Serial.println("[FAN2] ON"); digitalWrite(PIN_RELAY_FAN2,RELAY_ON);}
    _lfc=millis();
}

void stopFan(uint8_t n) {
    if(n==1){Serial.println("[FAN1] OFF"); digitalWrite(PIN_RELAY_FAN1,RELAY_OFF);}
    if(n==2){Serial.println("[FAN2] OFF"); digitalWrite(PIN_RELAY_FAN2,RELAY_OFF);}
}

void startAllFans(){startFan(1);delay(300);startFan(2);}
void stopAllFans() {stopFan(1);stopFan(2);}

void applyThresholdLogic(const SensorData &d, SystemState &s) {
    if(!d.dhtValid){Serial.println("[AUTO] Date invalide");return;}
    float t=d.temperature;
    // Ferestre
    if(t>=TEMP_OPEN_WINDOWS && !s.windowsOpen)  {openWindows(); s.windowsOpen=true;}
    if(t<=TEMP_CLOSE_WINDOWS && s.windowsOpen)  {closeWindows();s.windowsOpen=false;}
    // Ventilatoare
    if(t>=TEMP_START_FANS && !s.fan1Running)    {startAllFans();s.fan1Running=s.fan2Running=true;}
    if(t<=TEMP_STOP_FANS  &&  s.fan1Running)    {stopAllFans(); s.fan1Running=s.fan2Running=false;}
    // Alerte critice
    if(t>=TEMP_ALERT_HIGH){
        Serial.printf("[ALERT] T CRITICA: %.1f\n",t); s.alertActive=true;
        if(!s.windowsOpen){openWindows();s.windowsOpen=true;}
        if(!s.fan1Running){startAllFans();s.fan1Running=s.fan2Running=true;}
    }
    if(t<=TEMP_ALERT_LOW){
        Serial.printf("[ALERT] T MICA: %.1f\n",t); s.alertActive=true;
        if(s.windowsOpen){closeWindows();s.windowsOpen=false;}
        if(s.fan1Running){stopAllFans();s.fan1Running=s.fan2Running=false;}
    }
    if(d.soilValid && d.soilPct<SOIL_ALERT_DRY_PCT){
        Serial.printf("[ALERT] SOL USCAT: %.1f%%\n",d.soilPct); s.alertActive=true;
    }
}

void emergencyStop() {
    Serial.println("[EMERG] *** STOP ***");
    digitalWrite(PIN_RELAY_WINDOW1,RELAY_OFF); digitalWrite(PIN_RELAY_WINDOW2,RELAY_OFF);
    digitalWrite(PIN_RELAY_PUMP,RELAY_OFF);    digitalWrite(PIN_RELAY_FAN1,RELAY_OFF);
    digitalWrite(PIN_RELAY_FAN2,RELAY_OFF);    digitalWrite(PIN_LED_STATUS,LOW);
}

void handleSerialCommands(SystemState &s) {
    if(!Serial.available()) return;
    String c=Serial.readStringUntil('\n'); c.trim(); c.toUpperCase();
    if(c=="OPEN_WINDOWS")        {openWindows();         s.windowsOpen=true;}
    else if(c=="CLOSE_WINDOWS")  {closeWindows();        s.windowsOpen=false;}
    else if(c=="PUMP_ON")        {startPump(0);          s.pumpRunning=true;s.pumpDuration=0;}
    else if(c.startsWith("PUMP_ON ")) {
        unsigned long d=c.substring(8).toInt();
        startPump(d);s.pumpRunning=true;s.pumpStartTime=millis();s.pumpDuration=d;
    }
    else if(c=="PUMP_OFF")       {stopPump();            s.pumpRunning=false;}
    else if(c=="FAN1_ON")        {startFan(1);           s.fan1Running=true;}
    else if(c=="FAN1_OFF")       {stopFan(1);            s.fan1Running=false;}
    else if(c=="FAN2_ON")        {startFan(2);           s.fan2Running=true;}
    else if(c=="FAN2_OFF")       {stopFan(2);            s.fan2Running=false;}
    else if(c=="FANS_ON")        {startAllFans();        s.fan1Running=s.fan2Running=true;}
    else if(c=="FANS_OFF")       {stopAllFans();         s.fan1Running=s.fan2Running=false;}
    else if(c=="AUTO_ON")        {s.autoMode=true;       Serial.println("[CMD] Auto ON");}
    else if(c=="AUTO_OFF")       {s.autoMode=false;      Serial.println("[CMD] Auto OFF");}
    else if(c=="STOP_ALL")       {emergencyStop();s.windowsOpen=s.pumpRunning=s.fan1Running=s.fan2Running=false;}
    else if(c=="STATUS") Serial.printf("Win:%s Pump:%s F1:%s F2:%s Auto:%s\n",
        s.windowsOpen?"ON":"OFF",s.pumpRunning?"ON":"OFF",
        s.fan1Running?"ON":"OFF",s.fan2Running?"ON":"OFF",s.autoMode?"ON":"OFF");
    else if(c=="HELP") Serial.println(
        "Comenzi: OPEN_WINDOWS|CLOSE_WINDOWS|PUMP_ON [ms]|PUMP_OFF|"
        "FAN1_ON|FAN1_OFF|FAN2_ON|FAN2_OFF|FANS_ON|FANS_OFF|"
        "AUTO_ON|AUTO_OFF|STATUS|STOP_ALL|HELP");
    else Serial.println("[CMD] Necunoscut. Scrie HELP.");
}
