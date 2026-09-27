/**
 * =============================================================================
 * config.h  —  Greenhouse IoT System | Firmware Stage 1
 * =============================================================================
 * PURPOSE: Single source of truth for every constant in the firmware.
 *          Adjust pin numbers, timing, and thresholds here ONLY.
 *
 * USAGE:
 *   1. Set PIN_* to match your physical wiring (see schema_conexiuni.txt).
 *   2. Adjust TEMP_* thresholds for your crop.
 *   3. Run ./scripts/calibrate_soil.sh to get SOIL_ADC_DRY / SOIL_ADC_WET.
 *   4. Upload: ./scripts/flash.sh upload
 * =============================================================================
 */
#pragma once

// ── Identity ──────────────────────────────────────────────────────────────────
#define FIRMWARE_VERSION    "1.0.0"
#define DEVICE_ID           "SERA_001"  // Unique per physical device

// ── GPIO Pins ─────────────────────────────────────────────────────────────────
// IMPORTANT: GPIO 34, 35, 36, 39 on ESP32 are INPUT-ONLY — never OUTPUT.
#define PIN_DHT22           4    // DHT22 DATA; requires 10kΩ pull-up to 3.3V
#define PIN_SOIL_SENSOR     34   // Capacitive soil sensor analog output (ADC1_CH6)
#define PIN_RELAY_WINDOW1   16   // Window motor / linear actuator #1
#define PIN_RELAY_WINDOW2   17   // Window motor / linear actuator #2
#define PIN_RELAY_PUMP      18   // 220V solenoid valve (normally-closed for safety)
#define PIN_RELAY_FAN1      19   // 220V ventilation fan #1
#define PIN_RELAY_FAN2      21   // 220V ventilation fan #2
#define PIN_LED_STATUS      2    // Built-in LED; ON while pump relay is energised

// ── Sensor Type ───────────────────────────────────────────────────────────────
// DHT22 = AM2302: ±0.5°C, ±2-5% RH. Use DHT11 for the lower-cost blue sensor.
#define DHT_TYPE            DHT22

// ── Timing (milliseconds) ─────────────────────────────────────────────────────
// SENSOR_INTERVAL_MS  : How often to sample all sensors. 30s is a good balance.
// RELAY_DEBOUNCE_MS   : Minimum gap between two operations on the same relay.
//                       Prevents inrush current damage to motor windings.
// WINDOW_TRAVEL_TIME_MS: How long the window relay stays ON. Set to the time
//                       your actuator needs to travel fully open/closed.
//                       Relay de-energises automatically after this period.
#define SENSOR_INTERVAL_MS       30000UL   // 30 seconds
#define RELAY_DEBOUNCE_MS        2000UL    // 2 seconds
#define WINDOW_TRAVEL_TIME_MS    15000UL   // 15 seconds (adjust for your motor)

// ── Temperature Thresholds (°C) ───────────────────────────────────────────────
// Hysteresis: OPEN threshold > CLOSE threshold prevents rapid toggling.
// Example: window opens at 28°C and stays open until temperature drops to 24°C.
#define TEMP_OPEN_WINDOWS    28.0f   // Open windows above this temperature
#define TEMP_CLOSE_WINDOWS   24.0f   // Close windows below this temperature
#define TEMP_START_FANS      26.0f   // Start fans above this temperature
#define TEMP_STOP_FANS       23.0f   // Stop fans below this temperature
#define TEMP_ALERT_HIGH      35.0f   // CRITICAL: unconditionally open everything
#define TEMP_ALERT_LOW        5.0f   // CRITICAL: frost risk → close everything

// ── Humidity Thresholds (%) ───────────────────────────────────────────────────
// Currently used for alerts only; add hardware (humidifier relay) as needed.
#define HUMIDITY_ALERT_HIGH  90.0f   // Condensation / fungal disease risk
#define HUMIDITY_ALERT_LOW   30.0f   // Dehydration risk

// ── Soil Moisture Calibration ─────────────────────────────────────────────────
// ADC is 12-bit → values from 0 to 4095.
// Capacitive sensors are INVERSE: higher ADC = drier soil.
// Measure your specific sensor with calibrate_soil.sh and update here.
#define SOIL_ADC_DRY         3200    // Sensor in open air ≈ 0% moisture
#define SOIL_ADC_WET          800    // Sensor submerged in water ≈ 100% moisture
#define SOIL_ALERT_DRY_PCT   20.0f   // Alert when moisture falls below 20%

// ── Relay Logic ───────────────────────────────────────────────────────────────
// Most relay modules sold for ESP32 are ACTIVE-LOW:
//   LOW  signal → relay coil energised → contact CLOSED → device ON
//   HIGH signal → relay coil off       → contact OPEN  → device OFF
// Set RELAY_ACTIVE_LOW false if your relay board is ACTIVE-HIGH.
// RELAY_ON / RELAY_OFF expand to the correct voltage level automatically.
#define RELAY_ACTIVE_LOW     true
#define RELAY_ON             (RELAY_ACTIVE_LOW ? LOW  : HIGH)
#define RELAY_OFF            (RELAY_ACTIVE_LOW ? HIGH : LOW)

// ── Data Structures ───────────────────────────────────────────────────────────

/**
 * SensorData — snapshot of all sensor readings from one polling cycle.
 * Always check dhtValid / soilValid before using the numeric fields;
 * they are 0 when the sensor read failed, which could be confused with
 * a genuine 0 °C or 0% moisture reading.
 */
struct SensorData {
    float temperature;        // °C; 0.0 if dhtValid == false
    float humidity;           // %RH; 0.0 if dhtValid == false
    int   soilRaw;            // Raw 12-bit ADC (0–4095); 0 if soilValid == false
    float soilPct;            // Computed moisture 0–100%; 0.0 if soilValid == false
    bool  dhtValid;           // true = DHT22 returned non-NaN values
    bool  soilValid;          // true = ADC returned value in 0–4095 range
    unsigned long timestamp;  // millis() at moment of reading (used for logging)
};

/**
 * SystemState — mutable actuator state and operating mode.
 * This struct is passed by reference so any module can read or update the state.
 * pumpStartTime + pumpDuration support timed irrigation:
 *   if pumpDuration > 0, checkPumpTimeout() stops the pump automatically.
 */
struct SystemState {
    bool windowsOpen;          // Last commanded state of window relays
    bool pumpRunning;          // true = pump relay is currently energised
    bool fan1Running;          // true = fan 1 relay is energised
    bool fan2Running;          // true = fan 2 relay is energised
    bool autoMode;             // true = threshold logic runs every loop
    bool alertActive;          // true if an alert was triggered this cycle
    unsigned long pumpStartTime;  // millis() when pump was last started
    unsigned long pumpDuration;   // Requested run time in ms; 0 = unlimited
};
