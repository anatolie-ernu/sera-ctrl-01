/**
 * ============================================================
 * SERA INTELIGENTA - ETAPA 1
 * Fișier: actuators.cpp
 * Descriere: Implementare control relee și logică automată
 * ============================================================
 */

#include "actuators.h"
#include <Arduino.h>

// ─── Variabile interne de stare ───────────────────────────────
static bool _windowsOpen  = false;
static bool _pumpRunning  = false;
static bool _fan1Running  = false;
static bool _fan2Running  = false;
static unsigned long _lastWindowChange = 0;
static unsigned long _lastPumpChange   = 0;
static unsigned long _lastFanChange    = 0;

// ─── Inițializare pinii relee ────────────────────────────────
void initActuators() {
  pinMode(PIN_RELAY_WINDOW1, OUTPUT);
  pinMode(PIN_RELAY_WINDOW2, OUTPUT);
  pinMode(PIN_RELAY_PUMP,    OUTPUT);
  pinMode(PIN_RELAY_FAN1,    OUTPUT);
  pinMode(PIN_RELAY_FAN2,    OUTPUT);
  pinMode(PIN_LED_STATUS,    OUTPUT);

  // Stare inițială: toate releele OFF
  digitalWrite(PIN_RELAY_WINDOW1, RELAY_OFF);
  digitalWrite(PIN_RELAY_WINDOW2, RELAY_OFF);
  digitalWrite(PIN_RELAY_PUMP,    RELAY_OFF);
  digitalWrite(PIN_RELAY_FAN1,    RELAY_OFF);
  digitalWrite(PIN_RELAY_FAN2,    RELAY_OFF);
  digitalWrite(PIN_LED_STATUS,    LOW);

  Serial.println("[ACTUATORS] Toate releele inițializate - stare OFF");
  Serial.printf("[ACTUATORS] RELAY_ON=%d, RELAY_OFF=%d\n", RELAY_ON, RELAY_OFF);
}

// ─── Control ferestre ─────────────────────────────────────────
void openWindows() {
  unsigned long now = millis();
  if (_windowsOpen) {
    Serial.println("[WINDOWS] Deja deschise - ignorat");
    return;
  }
  if (now - _lastWindowChange < RELAY_DEBOUNCE_MS) {
    Serial.println("[WINDOWS] Debounce activ - ignorat");
    return;
  }
  Serial.println("[WINDOWS] >>> DESCHIDERE FERESTRE <<<");
  digitalWrite(PIN_RELAY_WINDOW1, RELAY_ON);
  digitalWrite(PIN_RELAY_WINDOW2, RELAY_ON);
  delay(WINDOW_TRAVEL_TIME_MS);   // Așteptăm ca motoarele să parcurgă distanța
  // Notă: În producție, folosiți un task non-blocking (timer/millis)
  digitalWrite(PIN_RELAY_WINDOW1, RELAY_OFF);
  digitalWrite(PIN_RELAY_WINDOW2, RELAY_OFF);
  _windowsOpen = true;
  _lastWindowChange = millis();
  Serial.println("[WINDOWS] Ferestre deschise complet.");
}

void closeWindows() {
  unsigned long now = millis();
  if (!_windowsOpen) {
    Serial.println("[WINDOWS] Deja închise - ignorat");
    return;
  }
  if (now - _lastWindowChange < RELAY_DEBOUNCE_MS) {
    Serial.println("[WINDOWS] Debounce activ - ignorat");
    return;
  }
  // IMPORTANT: motoarele de fereastră necesită sens invers de rotație
  // pentru închidere. Dacă motorul tău are un singur sens, adaptează pinii.
  // Această implementare presupune că un releu separat inversează sensul.
  Serial.println("[WINDOWS] >>> ÎNCHIDERE FERESTRE <<<");
  // Activăm releele în sens invers (implementare depinde de hardware)
  digitalWrite(PIN_RELAY_WINDOW1, RELAY_ON);
  digitalWrite(PIN_RELAY_WINDOW2, RELAY_ON);
  delay(WINDOW_TRAVEL_TIME_MS);
  digitalWrite(PIN_RELAY_WINDOW1, RELAY_OFF);
  digitalWrite(PIN_RELAY_WINDOW2, RELAY_OFF);
  _windowsOpen = false;
  _lastWindowChange = millis();
  Serial.println("[WINDOWS] Ferestre închise complet.");
}

bool areWindowsOpen() { return _windowsOpen; }

// ─── Control pompă ────────────────────────────────────────────
void startPump() {
  if (_pumpRunning) {
    Serial.println("[PUMP] Deja pornită - ignorat");
    return;
  }
  unsigned long now = millis();
  if (now - _lastPumpChange < RELAY_DEBOUNCE_MS) {
    Serial.println("[PUMP] Debounce activ - ignorat");
    return;
  }
  Serial.println("[PUMP] >>> PORNIRE POMPĂ / ROBINET <<<");
  digitalWrite(PIN_RELAY_PUMP, RELAY_ON);
  _pumpRunning = true;
  _lastPumpChange = millis();
  digitalWrite(PIN_LED_STATUS, HIGH);
}

void stopPump() {
  if (!_pumpRunning) {
    Serial.println("[PUMP] Deja oprită - ignorat");
    return;
  }
  Serial.println("[PUMP] >>> OPRIRE POMPĂ / ROBINET <<<");
  digitalWrite(PIN_RELAY_PUMP, RELAY_OFF);
  _pumpRunning = false;
  _lastPumpChange = millis();
  digitalWrite(PIN_LED_STATUS, LOW);
}

bool isPumpRunning() { return _pumpRunning; }

// ─── Control ventilatoare ─────────────────────────────────────
void startFan(uint8_t fanNumber) {
  unsigned long now = millis();
  if (now - _lastFanChange < RELAY_DEBOUNCE_MS) return;

  if (fanNumber == 1 && !_fan1Running) {
    Serial.println("[FAN1] >>> PORNIRE VENTILATOR 1 <<<");
    digitalWrite(PIN_RELAY_FAN1, RELAY_ON);
    _fan1Running = true;
    _lastFanChange = now;
  } else if (fanNumber == 2 && !_fan2Running) {
    Serial.println("[FAN2] >>> PORNIRE VENTILATOR 2 <<<");
    digitalWrite(PIN_RELAY_FAN2, RELAY_ON);
    _fan2Running = true;
    _lastFanChange = now;
  }
}

void stopFan(uint8_t fanNumber) {
  if (fanNumber == 1 && _fan1Running) {
    Serial.println("[FAN1] >>> OPRIRE VENTILATOR 1 <<<");
    digitalWrite(PIN_RELAY_FAN1, RELAY_OFF);
    _fan1Running = false;
  } else if (fanNumber == 2 && _fan2Running) {
    Serial.println("[FAN2] >>> OPRIRE VENTILATOR 2 <<<");
    digitalWrite(PIN_RELAY_FAN2, RELAY_OFF);
    _fan2Running = false;
  }
}

void startAllFans() {
  startFan(1);
  delay(500); // Pauză scurtă între porniri
  startFan(2);
}

void stopAllFans() {
  stopFan(1);
  stopFan(2);
}

// ─── Logică automată praguri ──────────────────────────────────
void applyThresholdLogic(const SensorData &data, SystemState &state) {
  if (!data.dhtValid) {
    Serial.println("[AUTO] Date invalide - logică automată suspendată");
    return;
  }

  float t = data.temperature;

  // ── Logică ferestre ──
  if (t >= TEMP_OPEN_WINDOWS && !state.windowsOpen) {
    Serial.printf("[AUTO] T=%.1f°C >= %.1f°C → Deschid ferestrele\n",
                  t, TEMP_OPEN_WINDOWS);
    openWindows();
    state.windowsOpen = true;
  } else if (t <= TEMP_CLOSE_WINDOWS && state.windowsOpen) {
    Serial.printf("[AUTO] T=%.1f°C <= %.1f°C → Închid ferestrele\n",
                  t, TEMP_CLOSE_WINDOWS);
    closeWindows();
    state.windowsOpen = false;
  }

  // ── Logică ventilatoare ──
  if (t >= TEMP_START_FANS && !state.fan1Running) {
    Serial.printf("[AUTO] T=%.1f°C >= %.1f°C → Pornesc ventilatoarele\n",
                  t, TEMP_START_FANS);
    startAllFans();
    state.fan1Running = true;
    state.fan2Running = true;
  } else if (t <= TEMP_STOP_FANS && state.fan1Running) {
    Serial.printf("[AUTO] T=%.1f°C <= %.1f°C → Opresc ventilatoarele\n",
                  t, TEMP_STOP_FANS);
    stopAllFans();
    state.fan1Running = false;
    state.fan2Running = false;
  }

  // ── Alerte critice ──
  if (t >= TEMP_ALERT_HIGH) {
    Serial.printf("[ALERT!] TEMPERATURA CRITICA: %.1f°C!\n", t);
    state.alertActive = true;
    // Asigurăm că ferestrele și ventilatoarele sunt deschise
    if (!state.windowsOpen) { openWindows(); state.windowsOpen = true; }
    if (!state.fan1Running)  { startAllFans(); state.fan1Running = state.fan2Running = true; }
  }
  if (t <= TEMP_ALERT_LOW) {
    Serial.printf("[ALERT!] TEMPERATURA PREA MICA: %.1f°C!\n", t);
    state.alertActive = true;
    // Închidem ferestrele și oprim ventilatoarele
    if (state.windowsOpen)  { closeWindows(); state.windowsOpen = false; }
    if (state.fan1Running)  { stopAllFans();  state.fan1Running = state.fan2Running = false; }
  }
}

// ─── Comenzi seriale pentru testare ──────────────────────────
void handleSerialCommands(SystemState &state) {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();

  Serial.println("[CMD] Comandă primită: " + cmd);

  if (cmd == "OPEN_WINDOWS")   { openWindows();  state.windowsOpen = true; }
  else if (cmd == "CLOSE_WINDOWS") { closeWindows(); state.windowsOpen = false; }
  else if (cmd == "PUMP_ON")   { startPump();    state.pumpRunning = true; }
  else if (cmd == "PUMP_OFF")  { stopPump();     state.pumpRunning = false; }
  else if (cmd == "FAN1_ON")   { startFan(1);   state.fan1Running = true; }
  else if (cmd == "FAN1_OFF")  { stopFan(1);    state.fan1Running = false; }
  else if (cmd == "FAN2_ON")   { startFan(2);   state.fan2Running = true; }
  else if (cmd == "FAN2_OFF")  { stopFan(2);    state.fan2Running = false; }
  else if (cmd == "AUTO_ON")   { state.autoMode = true;  Serial.println("[CMD] Mod automat ACTIVAT"); }
  else if (cmd == "AUTO_OFF")  { state.autoMode = false; Serial.println("[CMD] Mod automat DEZACTIVAT"); }
  else if (cmd == "STATUS") {
    Serial.println("=== STARE SISTEM ===");
    Serial.printf("Ferestre: %s\n",   state.windowsOpen ? "DESCHISE" : "INCHISE");
    Serial.printf("Pompa:    %s\n",   state.pumpRunning ? "PORNITA"  : "OPRITA");
    Serial.printf("Fan 1:    %s\n",   state.fan1Running ? "PORNIT"   : "OPRIT");
    Serial.printf("Fan 2:    %s\n",   state.fan2Running ? "PORNIT"   : "OPRIT");
    Serial.printf("Mod Auto: %s\n",   state.autoMode    ? "DA"       : "NU");
  }
  else if (cmd == "STOP_ALL") {
    emergencyStop();
    state.windowsOpen = state.pumpRunning = state.fan1Running = state.fan2Running = false;
  }
  else if (cmd == "HELP") {
    Serial.println("Comenzi disponibile:");
    Serial.println("  OPEN_WINDOWS / CLOSE_WINDOWS");
    Serial.println("  PUMP_ON / PUMP_OFF");
    Serial.println("  FAN1_ON / FAN1_OFF / FAN2_ON / FAN2_OFF");
    Serial.println("  AUTO_ON / AUTO_OFF");
    Serial.println("  STATUS / STOP_ALL / HELP");
  }
  else {
    Serial.println("[CMD] Comandă necunoscută. Scrie HELP pentru ajutor.");
  }
}

// ─── Oprire urgență ───────────────────────────────────────────
void emergencyStop() {
  Serial.println("[EMERGENCY] *** OPRIRE DE URGENTA - TOATE DISPOZITIVELE OFF ***");
  digitalWrite(PIN_RELAY_WINDOW1, RELAY_OFF);
  digitalWrite(PIN_RELAY_WINDOW2, RELAY_OFF);
  digitalWrite(PIN_RELAY_PUMP,    RELAY_OFF);
  digitalWrite(PIN_RELAY_FAN1,    RELAY_OFF);
  digitalWrite(PIN_RELAY_FAN2,    RELAY_OFF);
  digitalWrite(PIN_LED_STATUS,    LOW);
  _windowsOpen = _pumpRunning = _fan1Running = _fan2Running = false;
}
