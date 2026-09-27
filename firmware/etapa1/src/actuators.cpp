/**
 * actuators.cpp  —  Greenhouse IoT System | Firmware Stage 1
 * ─────────────────────────────────────────────────────────────────────────────
 * Controls the five relay channels and implements the autonomous threshold
 * logic that keeps the greenhouse safe even when the server is offline.
 *
 * Relay safety model
 * ─────────────────────────────────────────────────────────────────────────────
 * 1. Debounce: module-level timestamps (_lwc, _lpc, _lfc) record the last
 *    time each relay group was operated.  A new command is ignored if less
 *    than RELAY_DEBOUNCE_MS has elapsed.  This protects motor windings from
 *    inrush current caused by rapid toggling.
 *
 * 2. Window travel time: openWindows() and closeWindows() hold the relay ON
 *    for exactly WINDOW_TRAVEL_TIME_MS then release it.  The motor is therefore
 *    never left stalled against a mechanical stop.
 *    ⚠ This blocking delay is acceptable in Stage 1 (no WiFi tasks running).
 *    Stage 2+ should replace it with a millis()-based state machine or a
 *    FreeRTOS task to avoid blocking the MQTT keepalive.
 *
 * 3. Timed pump: startPump(durMs) stores the start time and requested duration
 *    in SystemState.  checkPumpTimeout() is called every loop and stops the
 *    pump when the time expires, preventing accidental flooding.
 *
 * Threshold hysteresis
 * ─────────────────────────────────────────────────────────────────────────────
 * Windows open at TEMP_OPEN_WINDOWS (28°C) but only close when temperature
 * drops below TEMP_CLOSE_WINDOWS (24°C).  This 4°C band prevents the windows
 * from flapping rapidly when temperature oscillates around a single setpoint.
 * The same principle applies to fan start/stop thresholds.
 */
#include "actuators.h"
#include <Arduino.h>

// ── Module-level debounce timestamps ──────────────────────────────────────────
// Separate timestamps for each relay group so a window command does not
// reset the pump debounce timer, for example.
static unsigned long _lwc = 0;   // Last Window Change
static unsigned long _lpc = 0;   // Last Pump Change
static unsigned long _lfc = 0;   // Last Fan Change

// ─────────────────────────────────────────────────────────────────────────────
void initActuators() {
    // Configure every relay control pin as a digital output
    uint8_t pins[] = {
        PIN_RELAY_WINDOW1, PIN_RELAY_WINDOW2,
        PIN_RELAY_PUMP,
        PIN_RELAY_FAN1, PIN_RELAY_FAN2,
        PIN_LED_STATUS
    };
    for (auto p : pins) pinMode(p, OUTPUT);

    // Set all relays to OFF (de-energised) at startup.
    // RELAY_OFF expands to HIGH for active-low modules (the common case).
    // This ensures no device accidentally activates on power-up.
    digitalWrite(PIN_RELAY_WINDOW1, RELAY_OFF);
    digitalWrite(PIN_RELAY_WINDOW2, RELAY_OFF);
    digitalWrite(PIN_RELAY_PUMP,    RELAY_OFF);
    digitalWrite(PIN_RELAY_FAN1,    RELAY_OFF);
    digitalWrite(PIN_RELAY_FAN2,    RELAY_OFF);
    digitalWrite(PIN_LED_STATUS,    LOW);

    Serial.println("[ACT] All relays initialised — OFF");
}

// ─────────────────────────────────────────────────────────────────────────────
// WINDOW CONTROL
// Both windows are operated simultaneously because they are in the same
// greenhouse zone.  If your greenhouse has independently controllable windows,
// split this into openWindow(uint8_t n) functions.
// ─────────────────────────────────────────────────────────────────────────────
void openWindows() {
    if (millis() - _lwc < RELAY_DEBOUNCE_MS) {
        Serial.println("[WIN] Debounce active — ignored");
        return;
    }
    Serial.println("[WIN] OPENING WINDOWS");
    // Energise both window relays simultaneously
    digitalWrite(PIN_RELAY_WINDOW1, RELAY_ON);
    digitalWrite(PIN_RELAY_WINDOW2, RELAY_ON);
    // Wait for the actuator to reach the fully-open position
    delay(WINDOW_TRAVEL_TIME_MS);
    // De-energise — motor must not be left stalled
    digitalWrite(PIN_RELAY_WINDOW1, RELAY_OFF);
    digitalWrite(PIN_RELAY_WINDOW2, RELAY_OFF);
    _lwc = millis();  // Record completion time for debounce
    Serial.println("[WIN] Windows fully open.");
}

void closeWindows() {
    if (millis() - _lwc < RELAY_DEBOUNCE_MS) return;
    Serial.println("[WIN] CLOSING WINDOWS");
    // NOTE: If your window motors require reversed polarity to close, you need
    // a second relay per motor (H-bridge configuration) or a relay with a
    // changeover contact wired to reverse the motor leads.
    // This implementation assumes the same relay direction closes the window
    // (e.g. spring-return actuators or a gravity-close design).
    digitalWrite(PIN_RELAY_WINDOW1, RELAY_ON);
    digitalWrite(PIN_RELAY_WINDOW2, RELAY_ON);
    delay(WINDOW_TRAVEL_TIME_MS);
    digitalWrite(PIN_RELAY_WINDOW1, RELAY_OFF);
    digitalWrite(PIN_RELAY_WINDOW2, RELAY_OFF);
    _lwc = millis();
    Serial.println("[WIN] Windows fully closed.");
}

// ─────────────────────────────────────────────────────────────────────────────
// PUMP / VALVE CONTROL
// The solenoid valve is wired to the Normally-Open contact of relay 3.
// This means a power failure de-energises the relay, closing the valve and
// stopping water flow — the safe fail-state for an irrigation system.
// ─────────────────────────────────────────────────────────────────────────────
void startPump(unsigned long durMs) {
    if (millis() - _lpc < RELAY_DEBOUNCE_MS) return;
    Serial.printf("[PUMP] ON (duration: %lu ms)\n", durMs);
    digitalWrite(PIN_RELAY_PUMP, RELAY_ON);
    digitalWrite(PIN_LED_STATUS, HIGH);   // Visual indicator
    _lpc = millis();
}

void stopPump() {
    if (millis() - _lpc < RELAY_DEBOUNCE_MS) return;
    Serial.println("[PUMP] OFF");
    digitalWrite(PIN_RELAY_PUMP, RELAY_OFF);
    digitalWrite(PIN_LED_STATUS, LOW);
    _lpc = millis();
}

// checkPumpTimeout() must be called every loop() iteration.
// It is a no-op when pumpDuration == 0 (manual / unlimited mode).
void checkPumpTimeout(SystemState &s) {
    if (s.pumpRunning && s.pumpDuration > 0) {
        if (millis() - s.pumpStartTime >= s.pumpDuration) {
            Serial.println("[PUMP] Timer expired — auto stop");
            stopPump();
            s.pumpRunning  = false;
            s.pumpDuration = 0;   // Reset so we don't stop again next cycle
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// FAN CONTROL
// Fans 1 and 2 can be operated independently, which allows staged ventilation:
// start fan 1 for moderate heat, add fan 2 for high heat.
// The 300 ms gap in startAllFans() reduces simultaneous inrush current.
// ─────────────────────────────────────────────────────────────────────────────
void startFan(uint8_t n) {
    if (millis() - _lfc < RELAY_DEBOUNCE_MS) return;
    if (n == 1) {
        Serial.println("[FAN1] ON");
        digitalWrite(PIN_RELAY_FAN1, RELAY_ON);
    } else if (n == 2) {
        Serial.println("[FAN2] ON");
        digitalWrite(PIN_RELAY_FAN2, RELAY_ON);
    }
    _lfc = millis();
}

void stopFan(uint8_t n) {
    // No debounce on stop — we always want to be able to turn off immediately
    if (n == 1) {
        Serial.println("[FAN1] OFF");
        digitalWrite(PIN_RELAY_FAN1, RELAY_OFF);
    } else if (n == 2) {
        Serial.println("[FAN2] OFF");
        digitalWrite(PIN_RELAY_FAN2, RELAY_OFF);
    }
}

void startAllFans() {
    startFan(1);
    delay(300);   // Stagger start to reduce simultaneous inrush current
    startFan(2);
}

void stopAllFans() {
    stopFan(1);
    stopFan(2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AUTONOMOUS THRESHOLD LOGIC
// This function is the heart of Stage 1 autonomy.  It is called every
// SENSOR_INTERVAL_MS when autoMode == true, and makes all relay decisions
// based purely on the most recent sensor data — no server connection required.
// ─────────────────────────────────────────────────────────────────────────────
void applyThresholdLogic(const SensorData &d, SystemState &s) {
    // Guard: if the DHT22 read failed this cycle, do not make actuator
    // decisions based on stale data from a previous (potentially old) reading.
    if (!d.dhtValid) {
        Serial.println("[AUTO] DHT invalid — threshold logic suspended");
        return;
    }

    float t = d.temperature;

    // ── Windows ──────────────────────────────────────────────────────────────
    // Open if hot; close if cool.  Hysteresis prevents rapid toggling.
    if (t >= TEMP_OPEN_WINDOWS && !s.windowsOpen) {
        Serial.printf("[AUTO] T=%.1f >= %.1f → Opening windows\n",
                      t, TEMP_OPEN_WINDOWS);
        openWindows();
        s.windowsOpen = true;
    } else if (t <= TEMP_CLOSE_WINDOWS && s.windowsOpen) {
        Serial.printf("[AUTO] T=%.1f <= %.1f → Closing windows\n",
                      t, TEMP_CLOSE_WINDOWS);
        closeWindows();
        s.windowsOpen = false;
    }

    // ── Fans ──────────────────────────────────────────────────────────────────
    if (t >= TEMP_START_FANS && !s.fan1Running) {
        Serial.printf("[AUTO] T=%.1f >= %.1f → Starting fans\n",
                      t, TEMP_START_FANS);
        startAllFans();
        s.fan1Running = s.fan2Running = true;
    } else if (t <= TEMP_STOP_FANS && s.fan1Running) {
        Serial.printf("[AUTO] T=%.1f <= %.1f → Stopping fans\n",
                      t, TEMP_STOP_FANS);
        stopAllFans();
        s.fan1Running = s.fan2Running = false;
    }

    // ── Critical high temperature ─────────────────────────────────────────────
    // Force everything open regardless of current state or debounce.
    if (t >= TEMP_ALERT_HIGH) {
        Serial.printf("[ALERT] CRITICAL HIGH: T=%.1f°C!\n", t);
        s.alertActive = true;
        if (!s.windowsOpen) { openWindows(); s.windowsOpen = true; }
        if (!s.fan1Running) { startAllFans(); s.fan1Running = s.fan2Running = true; }
    }

    // ── Critical low temperature (frost risk) ─────────────────────────────────
    // Close everything to retain heat; fans would accelerate heat loss.
    if (t <= TEMP_ALERT_LOW) {
        Serial.printf("[ALERT] CRITICAL LOW: T=%.1f°C — frost risk!\n", t);
        s.alertActive = true;
        if (s.windowsOpen)  { closeWindows(); s.windowsOpen = false; }
        if (s.fan1Running)  { stopAllFans();  s.fan1Running = s.fan2Running = false; }
    }

    // ── Dry soil alert ────────────────────────────────────────────────────────
    // Log and set the alert flag; actual irrigation is controlled by the
    // server-side scheduler (Stage 3+) or manual command.
    if (d.soilValid && d.soilPct < SOIL_ALERT_DRY_PCT) {
        Serial.printf("[ALERT] SOIL DRY: %.1f%%\n", d.soilPct);
        s.alertActive = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// EMERGENCY STOP
// Called from handleSerialCommands() via STOP_ALL, or can be called
// programmatically in any fault condition.
// Bypasses all debounce and timing logic — immediate cut.
// ─────────────────────────────────────────────────────────────────────────────
void emergencyStop() {
    Serial.println("[EMERG] *** EMERGENCY STOP — ALL RELAYS OFF ***");
    digitalWrite(PIN_RELAY_WINDOW1, RELAY_OFF);
    digitalWrite(PIN_RELAY_WINDOW2, RELAY_OFF);
    digitalWrite(PIN_RELAY_PUMP,    RELAY_OFF);
    digitalWrite(PIN_RELAY_FAN1,    RELAY_OFF);
    digitalWrite(PIN_RELAY_FAN2,    RELAY_OFF);
    digitalWrite(PIN_LED_STATUS,    LOW);
}

// ─────────────────────────────────────────────────────────────────────────────
// SERIAL COMMAND INTERFACE
// Allows manual testing and override via the PlatformIO Serial Monitor.
// Send commands as plain text terminated with newline (Enter key).
// ─────────────────────────────────────────────────────────────────────────────
void handleSerialCommands(SystemState &s) {
    if (!Serial.available()) return;  // Non-blocking: return immediately if empty

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    // ── Window commands ───────────────────────────────────────────────────────
    if      (cmd == "OPEN_WINDOWS")  { openWindows();  s.windowsOpen = true; }
    else if (cmd == "CLOSE_WINDOWS") { closeWindows(); s.windowsOpen = false; }

    // ── Pump commands ─────────────────────────────────────────────────────────
    // PUMP_ON          → runs until PUMP_OFF or power cycle
    // PUMP_ON <ms>     → runs for the specified number of milliseconds
    //   Example: "PUMP_ON 60000" waters for 60 seconds
    else if (cmd == "PUMP_ON") {
        startPump(0);
        s.pumpRunning  = true;
        s.pumpDuration = 0;
    }
    else if (cmd.startsWith("PUMP_ON ")) {
        unsigned long dur = cmd.substring(8).toInt();
        startPump(dur);
        s.pumpRunning   = true;
        s.pumpStartTime = millis();
        s.pumpDuration  = dur;
    }
    else if (cmd == "PUMP_OFF") { stopPump(); s.pumpRunning = false; }

    // ── Fan commands ──────────────────────────────────────────────────────────
    else if (cmd == "FAN1_ON")   { startFan(1);   s.fan1Running = true; }
    else if (cmd == "FAN1_OFF")  { stopFan(1);    s.fan1Running = false; }
    else if (cmd == "FAN2_ON")   { startFan(2);   s.fan2Running = true; }
    else if (cmd == "FAN2_OFF")  { stopFan(2);    s.fan2Running = false; }
    else if (cmd == "FANS_ON")   { startAllFans(); s.fan1Running = s.fan2Running = true; }
    else if (cmd == "FANS_OFF")  { stopAllFans();  s.fan1Running = s.fan2Running = false; }

    // ── Mode commands ─────────────────────────────────────────────────────────
    else if (cmd == "AUTO_ON")  { s.autoMode = true;  Serial.println("[CMD] Auto mode ON"); }
    else if (cmd == "AUTO_OFF") { s.autoMode = false; Serial.println("[CMD] Auto mode OFF — all manual"); }

    // ── Status and emergency ──────────────────────────────────────────────────
    else if (cmd == "STATUS") {
        Serial.printf(
            "Windows:%-4s Pump:%-4s Fan1:%-4s Fan2:%-4s Auto:%-3s Alert:%s\n",
            s.windowsOpen ? "OPEN" : "SHUT",
            s.pumpRunning ? "ON"   : "OFF",
            s.fan1Running ? "ON"   : "OFF",
            s.fan2Running ? "ON"   : "OFF",
            s.autoMode    ? "ON"   : "OFF",
            s.alertActive ? "YES"  : "NO"
        );
    }
    else if (cmd == "STOP_ALL") {
        emergencyStop();
        s.windowsOpen = s.pumpRunning = s.fan1Running = s.fan2Running = false;
    }
    else if (cmd == "HELP") {
        Serial.println(
            "Commands:\n"
            "  OPEN_WINDOWS | CLOSE_WINDOWS\n"
            "  PUMP_ON [duration_ms] | PUMP_OFF\n"
            "  FAN1_ON | FAN1_OFF | FAN2_ON | FAN2_OFF | FANS_ON | FANS_OFF\n"
            "  AUTO_ON | AUTO_OFF\n"
            "  STATUS | STOP_ALL | HELP"
        );
    }
    else {
        Serial.printf("[CMD] Unknown: '%s' — type HELP\n", cmd.c_str());
    }
}
