/**
 * ota_update.cpp — HTTPS OTA implementation with SHA-256 verification
 * Built on the ESP-IDF esp_ota_ops API (exposed through Arduino-ESP32).
 */
#include "ota_update.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

static char _version[16];

const char* otaCurrentVersion() {
    snprintf(_version, sizeof(_version), "%d.%d.%d",
             FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
    return _version;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * Compare two semver strings. @return true if `remote` is strictly newer.
 * Prevents replay attacks where an attacker pushes an OLD vulnerable image.
 */
static bool isNewerVersion(const char* remote) {
    int rM=0, rm=0, rp=0;
    sscanf(remote, "%d.%d.%d", &rM, &rm, &rp);
    long cur = FIRMWARE_VERSION_MAJOR * 1000000L
             + FIRMWARE_VERSION_MINOR * 1000L
             + FIRMWARE_VERSION_PATCH;
    long rem = rM * 1000000L + rm * 1000L + rp;
    return rem > cur;
}

// ─────────────────────────────────────────────────────────────────────────────
bool otaPerform(const char* url, const char* version, const char* sha256hex) {
    Serial.printf("[OTA] Request: %s → v%s\n", otaCurrentVersion(), version);

    // ── Guard 1: only accept strictly newer versions ─────────────────────────
    if (!isNewerVersion(version)) {
        Serial.println("[OTA] Rejected — version is not newer (anti-rollback)");
        return false;
    }

    // ── Guard 2: HTTPS only ──────────────────────────────────────────────────
    if (strncmp(url, "https://", 8) != 0) {
        Serial.println("[OTA] Rejected — only HTTPS URLs are accepted");
        return false;
    }

    WiFiClientSecure client;
    // PRODUCTION: pin your update server's CA certificate here:
    //   client.setCACert(UPDATE_SERVER_CA_PEM);
    // setInsecure() is acceptable ONLY for the development server.
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, url)) { Serial.println("[OTA] http.begin failed"); return false; }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[OTA] HTTP %d\n", code);
        http.end();
        return false;
    }

    int totalSize = http.getSize();
    Serial.printf("[OTA] Image size: %d bytes\n", totalSize);

    // Update.begin() selects the inactive OTA partition automatically
    if (!Update.begin(totalSize)) {
        Serial.printf("[OTA] begin failed: %s\n", Update.errorString());
        http.end();
        return false;
    }

    // ── Stream download with incremental SHA-256 ─────────────────────────────
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);   // 0 = SHA-256 (not SHA-224)

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[1024];
    int written = 0;
    unsigned long lastProgress = 0;

    while (http.connected() && written < totalSize) {
        size_t avail = stream->available();
        if (avail) {
            int n = stream->readBytes(buf, min(avail, sizeof(buf)));
            mbedtls_sha256_update(&sha, buf, n);   // Hash as we go
            if (Update.write(buf, n) != (size_t)n) {
                Serial.printf("[OTA] write failed: %s\n", Update.errorString());
                Update.abort(); http.end(); return false;
            }
            written += n;
            if (millis() - lastProgress > 3000) {
                lastProgress = millis();
                Serial.printf("[OTA] %d%%\n", written * 100 / totalSize);
            }
        }
        delay(1);
    }
    http.end();

    // ── Verify SHA-256 against the manifest ─────────────────────────────────
    uint8_t digest[32];
    mbedtls_sha256_finish(&sha, digest);
    mbedtls_sha256_free(&sha);

    char hex[65];
    for (int i = 0; i < 32; i++) sprintf(hex + i * 2, "%02x", digest[i]);

    if (strcasecmp(hex, sha256hex) != 0) {
        Serial.println("[OTA] SHA-256 MISMATCH — image rejected!");
        Serial.printf("[OTA]   expected: %s\n", sha256hex);
        Serial.printf("[OTA]   actual:   %s\n", hex);
        Update.abort();
        return false;
    }
    Serial.println("[OTA] SHA-256 verified OK");

    if (!Update.end(true)) {
        Serial.printf("[OTA] end failed: %s\n", Update.errorString());
        return false;
    }

    Serial.println("[OTA] Update written — rebooting into new firmware");
    delay(500);
    ESP.restart();
    return true;  // Not reached
}

// ─────────────────────────────────────────────────────────────────────────────
void otaMarkValid() {
    // Tell the bootloader the running image works.  Without this call,
    // the next reset rolls back to the previous partition.
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY) {
        esp_ota_mark_app_valid_cancel_rollback();
        Serial.println("[OTA] Firmware marked VALID — rollback cancelled");
    }
}
