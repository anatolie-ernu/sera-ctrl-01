/**
 * factory_test.h — Sera Inteligenta v3.0 | Firmware PRODUCTIE
 * ─────────────────────────────────────────────────────────────────────────────
 * End-of-line factory test mode — run on EVERY unit before it ships.
 *
 * Activation: send "FACTORY_TEST" over serial within 5 s of boot
 * (the production test jig does this automatically), or hold the BOOT
 * button during power-up.
 *
 * Test sequence (≈20 s total):
 *   1. Cycle each relay ON 500 ms → OFF (operator hears 5 clicks, jig
 *      verifies continuity on each output)
 *   2. Read SHT31 — PASS if plausible (-10..50 °C, 5..100 %RH)
 *   3. Read soil ADC channel — PASS if mid-rail with test resistor fitted
 *   4. WiFi scan — PASS if ≥1 network visible (antenna check)
 *   5. Print machine-readable report:  TEST_RESULT;<id>;PASS/FAIL;<details>
 *      The jig parses this line and prints the serial-number label.
 */
#pragma once

/** @return true if factory test mode was requested at boot. */
bool factoryTestRequested();

/** Run the full test sequence, print the report, halt (power-cycle to exit). */
void runFactoryTest();
