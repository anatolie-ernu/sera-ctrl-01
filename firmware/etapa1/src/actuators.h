/**
 * actuators.h  —  Greenhouse IoT System | Firmware Stage 1
 * ─────────────────────────────────────────────────────────────────────────────
 * Public interface for relay control, autonomous threshold logic, and the
 * Serial-based command interface used during development and testing.
 *
 * Implemented in: actuators.cpp
 * Safety guarantees:
 *   • Every relay function checks RELAY_DEBOUNCE_MS before switching.
 *   • Window relays de-energise automatically after WINDOW_TRAVEL_TIME_MS.
 *   • emergencyStop() cuts power to every relay immediately.
 */
#pragma once
#include "config.h"

// ── Initialisation ────────────────────────────────────────────────────────────
/** Configure all relay pins as OUTPUT and set them to RELAY_OFF state. */
void initActuators();

// ── Window control ────────────────────────────────────────────────────────────
/**
 * Energise both window relays for WINDOW_TRAVEL_TIME_MS then release.
 * Skipped silently if debounce timer has not elapsed.
 * NOTE: This function blocks for WINDOW_TRAVEL_TIME_MS (default 15 s).
 *       Stage 3+ replaces this with a non-blocking FreeRTOS task.
 */
void openWindows();
void closeWindows();

// ── Pump / valve control ──────────────────────────────────────────────────────
/**
 * Start the pump relay.
 * @param durMs Run duration in milliseconds. Pass 0 for unlimited (manual stop
 *              required via stopPump() or PUMP_OFF serial command).
 *              When durMs > 0, checkPumpTimeout() will stop the pump
 *              automatically when the duration has elapsed.
 */
void startPump(unsigned long durMs = 0);
void stopPump();

// ── Fan control ───────────────────────────────────────────────────────────────
/**
 * @param n Fan number: 1 or 2. Pass 0 via startAllFans() / stopAllFans().
 */
void startFan(uint8_t n);
void stopFan(uint8_t n);
void startAllFans();   // Starts fan 1 then fan 2 with a 300 ms gap (inrush)
void stopAllFans();

// ── Emergency ─────────────────────────────────────────────────────────────────
/** De-energise every relay immediately. No debounce check. */
void emergencyStop();

// ── Autonomous logic ──────────────────────────────────────────────────────────
/**
 * Evaluate the current sensor readings against all configured thresholds and
 * switch relays as needed.  Called every loop when systemState.autoMode == true.
 * Skips processing and logs a warning if data.dhtValid == false.
 */
void applyThresholdLogic(const SensorData &d, SystemState &s);

/**
 * Check whether a timed pump run has expired.  Call every loop.
 * Stops the pump and resets pumpDuration to 0 when the time is up.
 */
void checkPumpTimeout(SystemState &s);

// ── Development / testing ─────────────────────────────────────────────────────
/**
 * Read one line from Serial and execute the corresponding command.
 * Non-blocking: returns immediately if no data is available.
 * Type HELP in Serial Monitor for the full command list.
 */
void handleSerialCommands(SystemState &s);
