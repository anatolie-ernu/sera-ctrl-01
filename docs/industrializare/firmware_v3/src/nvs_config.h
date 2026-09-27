/**
 * nvs_config.h — Sera Inteligenta v3.0 | Firmware PRODUCTIE
 * ─────────────────────────────────────────────────────────────────────────────
 * Persistent configuration storage in ESP32 NVS (Non-Volatile Storage).
 *
 * WHY: In production, NOTHING can be hardcoded. WiFi credentials come from
 * the provisioning portal; MQTT settings and thresholds can be updated
 * remotely and must survive power cycles.  NVS is flash-backed key-value
 * storage with wear leveling — safe for ~100k write cycles per sector.
 *
 * All runtime configuration lives in the DeviceConfig struct.  Load once at
 * boot with cfgLoad(); save with cfgSave() after any change.
 */
#pragma once
#include <Arduino.h>

struct DeviceConfig {
    // ── Identity ──────────────────────────────────────────────
    char device_id[32];      // Unique per unit, e.g. "SERA-A1B2C3" (from MAC)
    char device_name[48];    // User-friendly name set during provisioning

    // ── Network (set by provisioning portal, NEVER hardcoded) ─
    char wifi_ssid[33];
    char wifi_pass[65];
    char mqtt_host[64];
    uint16_t mqtt_port;
    char mqtt_user[33];
    char mqtt_pass[65];
    bool mqtt_tls;           // true = MQTTS on 8883 with CA validation

    // ── Thresholds (updatable via MQTT cmd/config) ────────────
    float temp_open_windows;
    float temp_close_windows;
    float temp_start_fans;
    float temp_stop_fans;
    float temp_alert_high;
    float temp_alert_low;
    float soil_alert_dry_pct;
    int   soil_adc_dry;      // Per-unit factory calibration value
    int   soil_adc_wet;

    // ── State flags ───────────────────────────────────────────
    bool provisioned;        // false until first successful provisioning
    uint32_t boot_count;     // Incremented every boot (diagnostics)
};

extern DeviceConfig cfg;     // Global config instance, defined in nvs_config.cpp

/** Load config from NVS. Applies factory defaults on first boot. */
void cfgLoad();

/** Persist the current cfg struct to NVS. Call after every change. */
void cfgSave();

/** Erase all config and reboot — triggered by 10s button hold (factory reset). */
void cfgFactoryReset();

/** Generate the unique device_id from the ESP32 MAC address. */
void cfgGenerateDeviceId();
