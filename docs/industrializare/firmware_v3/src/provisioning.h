/**
 * provisioning.h — Sera Inteligenta v3.0 | Firmware PRODUCTIE
 * ─────────────────────────────────────────────────────────────────────────────
 * SoftAP captive-portal provisioning — how the CUSTOMER configures the device.
 *
 * Flow:
 *   1. Device boots unprovisioned (or BOOT button held 5 s at power-on).
 *   2. ESP32 starts an open access point named "SERA-XXXXXX" (device_id).
 *   3. Customer connects with phone; captive portal opens automatically
 *      (DNS server redirects every hostname to 192.168.4.1).
 *   4. Customer fills in: WiFi SSID + password, server address, device name.
 *   5. Config is saved to NVS, device reboots and connects normally.
 *
 * The portal is a single self-contained HTML page served from PROGMEM —
 * no SPIFFS dependency, nothing to flash separately in production.
 */
#pragma once
#include <Arduino.h>

/** @return true if provisioning is needed (no stored WiFi credentials). */
bool provisioningNeeded();

/**
 * Run the blocking provisioning portal.  Returns only after the customer
 * saves a configuration (device then reboots) or after a 10-minute timeout
 * (device reboots and retries with whatever config exists).
 */
void runProvisioningPortal();
