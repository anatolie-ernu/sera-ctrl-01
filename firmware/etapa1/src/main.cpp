/**
 * main.cpp  —  Greenhouse IoT System | Firmware Stage 1
 * ─────────────────────────────────────────────────────────────────────────────
 * Entry point for the Arduino framework.  setup() runs once after power-on;
 * loop() repeats indefinitely.
 *
 * Stage 1 responsibilities
 * ─────────────────────────────────────────────────────────────────────────────
 * • Read temperature, humidity, and soil moisture every 30 seconds.
 * • Apply autonomous threshold logic (windows, fans, alerts).
 * • Support manual override via Serial commands (type HELP in monitor).
 * • Check pump timeout each loop so timed irrigation stops correctly.
 *
 * Stage 2 additions (see firmware/etapa2/)
 * ─────────────────────────────────────────────────────────────────────────────
 * • WiFi connection with retry
 * • MQTT publish / subscribe
 * • NTP time synchronisation
 * • Heartbeat to server every 60 s
 */
#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "actuators.h"

// ── Global state ─────────────────────────────────────────────────────────────
// Declared at file scope so both modules can access them.
// Initialise all fields explicitly — C++ does not zero-initialise structs.
SensorData  sD = {
    .temperature = 0.0f, .humidity = 0.0f,
    .soilRaw = 0, .soilPct = 0.0f,
    .dhtValid = false, .soilValid = false,
    .timestamp = 0
};
SystemState sS = {
    .windowsOpen = false, .pumpRunning = false,
    .fan1Running = false,  .fan2Running = false,
    .autoMode    = true,   // Start in autonomous mode
    .alertActive = false,
    .pumpStartTime = 0,    .pumpDuration = 0
};

// Timestamp of the last sensor read cycle (millis-based non-blocking timer)
unsigned long lastSensorRead = 0;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);   // Allow USB serial to enumerate on the host

    Serial.println(F("\n╔═══════════════════════════════╗"));
    Serial.println(F(  "║  GREENHOUSE IoT v1.0          ║"));
    Serial.printf(     "║  Device:  %-20s ║\n", DEVICE_ID);
    Serial.printf(     "║  Firmware: %-19s ║\n", FIRMWARE_VERSION);
    Serial.println(F(  "╚═══════════════════════════════╝"));

    initActuators();   // Set up GPIO pins, all relays → OFF
    initSensors();     // Start DHT22, configure ADC pin, wait 2 s

    Serial.println(F("[MAIN] System ready. Autonomous mode ON."));
    Serial.println(F("[MAIN] Type HELP in Serial Monitor for commands."));
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── Periodic sensor read (non-blocking timer) ─────────────────────────────
    // Using millis() instead of delay() keeps the loop responsive for serial
    // commands and (in Stage 2) for the MQTT keepalive.
    if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
        lastSensorRead = now;
        readAllSensors(sD);   // Populate sD fields from hardware
        printSensorData(sD);  // Print formatted table to Serial
        sS.alertActive = false; // Reset alert flag for this new cycle
    }

    // ── Autonomous threshold control ─────────────────────────────────────────
    if (sS.autoMode) {
        applyThresholdLogic(sD, sS);
    }

    // ── Timed pump watchdog ───────────────────────────────────────────────────
    // Stops the pump if its requested duration has elapsed.
    // Must be called every loop — its overhead is negligible (a few clock cycles).
    checkPumpTimeout(sS);

    // ── Serial command interface ──────────────────────────────────────────────
    // Non-blocking: reads one line if available, otherwise returns immediately.
    handleSerialCommands(sS);

    delay(100);   // Yield 100 ms — prevents watchdog resets and reduces power
}
