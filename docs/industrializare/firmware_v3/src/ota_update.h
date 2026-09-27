/**
 * ota_update.h — Sera Inteligenta v3.0 | Firmware PRODUCTIE
 * ─────────────────────────────────────────────────────────────────────────────
 * Over-The-Air firmware updates with automatic rollback.
 *
 * Update flow:
 *   1. Backend publishes to greenhouse/<id>/cmd/ota:
 *        {"url":"https://updates.example.md/sera-ctrl/v3.1.0.bin",
 *         "version":"3.1.0", "sha256":"<hex>"}
 *   2. Device downloads over HTTPS into the INACTIVE app partition
 *      (A/B scheme — the running firmware is never touched).
 *   3. SHA-256 of the downloaded image is verified against the manifest.
 *   4. Boot partition is switched; device reboots into the new firmware.
 *   5. The new firmware must call otaMarkValid() after it successfully
 *      connects to MQTT.  If it crashes before that, the bootloader
 *      automatically ROLLS BACK to the previous partition.
 *
 * This guarantees a fleet can never be bricked by a bad update.
 */
#pragma once
#include <Arduino.h>

/**
 * Start an OTA update.  Blocking — takes 30–90 s on a typical connection.
 * @param url      HTTPS URL of the .bin image
 * @param version  Semver string of the new image ("3.1.0")
 * @param sha256   Expected SHA-256 hex digest of the image
 * @return true if the update was applied (device reboots before returning
 *         in the success path); false on any verification failure.
 */
bool otaPerform(const char* url, const char* version, const char* sha256);

/**
 * Confirm the currently running firmware is healthy.
 * MUST be called once after the first successful MQTT connection following
 * an update.  Until called, the bootloader treats the image as unverified
 * and will roll back on the next reset.
 */
void otaMarkValid();

/** @return the running firmware version string, e.g. "3.0.0" */
const char* otaCurrentVersion();
