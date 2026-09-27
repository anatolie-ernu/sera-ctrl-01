/**
 * provisioning.cpp — SoftAP captive portal implementation
 *
 * Uses the standard WebServer + DNSServer combo.  The DNS server answers
 * every query with 192.168.4.1 so phones detect a captive portal and open
 * the configuration page automatically (Android "Sign in to network",
 * iOS portal sheet).
 */
#include "provisioning.h"
#include "nvs_config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

static WebServer server(80);
static DNSServer dns;

// ── The configuration page — single file, served from flash ──────────────────
// Kept minimal and dependency-free: must render on any phone browser.
static const char PORTAL_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html><html lang="ro"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configurare Sera</title>
<style>
 body{font-family:system-ui,Arial;background:#f0f4f0;margin:0;padding:20px}
 .card{max-width:420px;margin:auto;background:#fff;border-radius:12px;
       box-shadow:0 2px 12px rgba(0,0,0,.1);padding:24px}
 h1{color:#1f6e3a;font-size:22px;margin:0 0 4px}
 p.sub{color:#666;font-size:13px;margin:0 0 20px}
 label{display:block;font-size:13px;font-weight:600;color:#333;margin:14px 0 4px}
 input,select{width:100%;padding:10px;border:1px solid #ccc;border-radius:8px;
       font-size:15px;box-sizing:border-box}
 button{width:100%;margin-top:22px;padding:13px;background:#1f6e3a;color:#fff;
       border:0;border-radius:8px;font-size:16px;font-weight:600}
 .id{font-family:monospace;background:#eef;padding:2px 6px;border-radius:4px}
</style></head><body><div class="card">
<h1>🌿 Configurare Sera</h1>
<p class="sub">Dispozitiv: <span class="id">%DEVICE_ID%</span> · HW %HW_REV% · FW %FW_VER%</p>
<form method="POST" action="/save">
 <label>Nume dispozitiv</label>
 <input name="name" maxlength="47" placeholder="Sera din gradina" required>
 <label>Retea WiFi (SSID)</label>
 <input name="ssid" maxlength="32" required>
 <label>Parola WiFi</label>
 <input name="pass" type="password" maxlength="64">
 <label>Server (adresa MQTT)</label>
 <input name="mhost" maxlength="63" placeholder="sera.exemplu.md" required>
 <label>Port</label>
 <input name="mport" type="number" value="8883" min="1" max="65535">
 <label>Utilizator MQTT</label>
 <input name="muser" maxlength="32">
 <label>Parola MQTT</label>
 <input name="mpass" type="password" maxlength="64">
 <button type="submit">Salveaza si reporneste</button>
</form></div></body></html>
)HTML";

// ─────────────────────────────────────────────────────────────────────────────
bool provisioningNeeded() {
    // Provision if never configured OR if WiFi SSID is empty
    return !cfg.provisioned || strlen(cfg.wifi_ssid) == 0;
}

// ── HTTP handlers ─────────────────────────────────────────────────────────────
static void handleRoot() {
    String page = FPSTR(PORTAL_HTML);
    page.replace("%DEVICE_ID%", cfg.device_id);
    page.replace("%HW_REV%",    HW_REVISION);
    char fw[16];
    snprintf(fw, sizeof(fw), "%d.%d.%d",
             FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
    page.replace("%FW_VER%", fw);
    server.send(200, "text/html", page);
}

static void handleSave() {
    // Copy submitted values into the global config struct
    strlcpy(cfg.device_name, server.arg("name").c_str(),  sizeof(cfg.device_name));
    strlcpy(cfg.wifi_ssid,   server.arg("ssid").c_str(),  sizeof(cfg.wifi_ssid));
    strlcpy(cfg.wifi_pass,   server.arg("pass").c_str(),  sizeof(cfg.wifi_pass));
    strlcpy(cfg.mqtt_host,   server.arg("mhost").c_str(), sizeof(cfg.mqtt_host));
    cfg.mqtt_port = server.arg("mport").toInt();
    strlcpy(cfg.mqtt_user,   server.arg("muser").c_str(), sizeof(cfg.mqtt_user));
    strlcpy(cfg.mqtt_pass,   server.arg("mpass").c_str(), sizeof(cfg.mqtt_pass));
    cfg.provisioned = true;
    cfgSave();

    server.send(200, "text/html",
        "<html><body style='font-family:Arial;text-align:center;padding:40px'>"
        "<h2 style='color:#1f6e3a'>✓ Salvat!</h2>"
        "<p>Dispozitivul reporneste si se conecteaza la reteaua dvs...</p>"
        "</body></html>");
    delay(1500);
    ESP.restart();   // Reboot into normal operation with the new config
}

// Captive-portal detection endpoints — answer with a redirect so the
// phone OS pops the portal automatically
static void handleCaptive() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
}

// ─────────────────────────────────────────────────────────────────────────────
void runProvisioningPortal() {
    Serial.println("[PROV] Starting provisioning portal...");

    // Open AP named after the unique device ID — customer identifies their unit
    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg.device_id);   // Open network; portal is local-only & short-lived
    delay(100);
    Serial.printf("[PROV] AP \"%s\" at %s\n",
                  cfg.device_id, WiFi.softAPIP().toString().c_str());

    // Wildcard DNS: every lookup resolves to us → captive portal detection
    dns.start(53, "*", WiFi.softAPIP());

    server.on("/",     HTTP_GET,  handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    // OS-specific captive portal probes:
    server.on("/generate_204",        handleCaptive);  // Android
    server.on("/hotspot-detect.html", handleCaptive);  // iOS / macOS
    server.on("/connecttest.txt",     handleCaptive);  // Windows
    server.onNotFound(handleCaptive);
    server.begin();

    // Blocking loop with a 10-minute safety timeout.  If nobody configures
    // the device, reboot — perhaps WiFi is back or the button-hold was accidental.
    unsigned long start = millis();
    while (millis() - start < 10UL * 60UL * 1000UL) {
        dns.processNextRequest();
        server.handleClient();
        delay(2);
    }
    Serial.println("[PROV] Portal timeout — rebooting");
    ESP.restart();
}
