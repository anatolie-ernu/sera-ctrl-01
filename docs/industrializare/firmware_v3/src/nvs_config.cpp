/**
 * nvs_config.cpp — Persistent configuration implementation
 * Uses the Preferences library (Arduino wrapper over ESP-IDF NVS).
 */
#include "nvs_config.h"
#include <Preferences.h>
#include <WiFi.h>

DeviceConfig cfg;                    // Global instance
static Preferences prefs;
static const char* NS = "sera";      // NVS namespace — all keys live here

// ── Factory defaults — applied when NVS is empty (first boot) ────────────────
static void applyDefaults() {
    memset(&cfg, 0, sizeof(cfg));
    cfgGenerateDeviceId();
    strlcpy(cfg.device_name, "Sera", sizeof(cfg.device_name));
    cfg.mqtt_port          = 8883;     // TLS by default in production
    cfg.mqtt_tls           = true;
    cfg.temp_open_windows  = 28.0f;
    cfg.temp_close_windows = 24.0f;
    cfg.temp_start_fans    = 26.0f;
    cfg.temp_stop_fans     = 23.0f;
    cfg.temp_alert_high    = 35.0f;
    cfg.temp_alert_low     = 5.0f;
    cfg.soil_alert_dry_pct = 20.0f;
    cfg.soil_adc_dry       = 3200;     // Overwritten by factory calibration
    cfg.soil_adc_wet       = 800;
    cfg.provisioned        = false;
}

void cfgGenerateDeviceId() {
    // Last 3 bytes of the factory MAC give a unique, stable serial suffix
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(cfg.device_id, sizeof(cfg.device_id),
             "SERA-%02X%02X%02X", mac[3], mac[4], mac[5]);
}

void cfgLoad() {
    prefs.begin(NS, /* readOnly */ false);

    if (!prefs.getBool("init", false)) {
        // First boot ever — write defaults
        applyDefaults();
        cfgSave();
        prefs.putBool("init", true);
        Serial.println("[NVS] First boot — factory defaults applied");
    } else {
        applyDefaults();   // Start from defaults, then overlay stored values
        prefs.getString("dev_name",  cfg.device_name, sizeof(cfg.device_name));
        prefs.getString("w_ssid",    cfg.wifi_ssid,   sizeof(cfg.wifi_ssid));
        prefs.getString("w_pass",    cfg.wifi_pass,   sizeof(cfg.wifi_pass));
        prefs.getString("m_host",    cfg.mqtt_host,   sizeof(cfg.mqtt_host));
        cfg.mqtt_port  = prefs.getUShort("m_port", 8883);
        prefs.getString("m_user",    cfg.mqtt_user,   sizeof(cfg.mqtt_user));
        prefs.getString("m_pass",    cfg.mqtt_pass,   sizeof(cfg.mqtt_pass));
        cfg.mqtt_tls   = prefs.getBool("m_tls", true);
        cfg.temp_open_windows  = prefs.getFloat("t_open",  28.0f);
        cfg.temp_close_windows = prefs.getFloat("t_close", 24.0f);
        cfg.temp_start_fans    = prefs.getFloat("t_fon",   26.0f);
        cfg.temp_stop_fans     = prefs.getFloat("t_foff",  23.0f);
        cfg.temp_alert_high    = prefs.getFloat("t_ahi",   35.0f);
        cfg.temp_alert_low     = prefs.getFloat("t_alo",   5.0f);
        cfg.soil_alert_dry_pct = prefs.getFloat("s_dry",   20.0f);
        cfg.soil_adc_dry       = prefs.getInt("s_adcd", 3200);
        cfg.soil_adc_wet       = prefs.getInt("s_adcw", 800);
        cfg.provisioned        = prefs.getBool("prov", false);
    }

    // Boot counter — useful for diagnosing crash loops in the field
    cfg.boot_count = prefs.getUInt("boots", 0) + 1;
    prefs.putUInt("boots", cfg.boot_count);

    Serial.printf("[NVS] Config loaded — %s, boot #%u, provisioned=%d\n",
                  cfg.device_id, cfg.boot_count, cfg.provisioned);
}

void cfgSave() {
    prefs.putString("dev_name", cfg.device_name);
    prefs.putString("w_ssid",   cfg.wifi_ssid);
    prefs.putString("w_pass",   cfg.wifi_pass);
    prefs.putString("m_host",   cfg.mqtt_host);
    prefs.putUShort("m_port",   cfg.mqtt_port);
    prefs.putString("m_user",   cfg.mqtt_user);
    prefs.putString("m_pass",   cfg.mqtt_pass);
    prefs.putBool  ("m_tls",    cfg.mqtt_tls);
    prefs.putFloat ("t_open",   cfg.temp_open_windows);
    prefs.putFloat ("t_close",  cfg.temp_close_windows);
    prefs.putFloat ("t_fon",    cfg.temp_start_fans);
    prefs.putFloat ("t_foff",   cfg.temp_stop_fans);
    prefs.putFloat ("t_ahi",    cfg.temp_alert_high);
    prefs.putFloat ("t_alo",    cfg.temp_alert_low);
    prefs.putFloat ("s_dry",    cfg.soil_alert_dry_pct);
    prefs.putInt   ("s_adcd",   cfg.soil_adc_dry);
    prefs.putInt   ("s_adcw",   cfg.soil_adc_wet);
    prefs.putBool  ("prov",     cfg.provisioned);
    Serial.println("[NVS] Config saved");
}

void cfgFactoryReset() {
    Serial.println("[NVS] *** FACTORY RESET — erasing all config ***");
    prefs.clear();           // Wipe the entire namespace
    prefs.end();
    delay(500);
    ESP.restart();           // Reboot into provisioning mode
}
