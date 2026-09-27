/**
 * hardware.h — Pin map for production PCB SERA-CTRL-01 rev A
 * ─────────────────────────────────────────────────────────────────────────────
 * ⚠ This file mirrors the PCB schematic.  If the layout changes, update
 *   BOTH this file and HW_REVISION in platformio.ini.
 *
 * Differences vs the DevKit prototype:
 *   • DHT22 replaced by SHT31 on I2C (GPIO21=SDA, GPIO22=SCL)
 *   • RS485 port added (MAX3485) for industrial Modbus soil sensors
 *   • Dedicated status LEDs and BOOT/RESET buttons routed to the panel
 */
#pragma once

// ── Relay outputs (via ULN2003A driver, channels 1–5) ────────
#define PIN_RELAY_WINDOW1   16   // K1 — window motor 1
#define PIN_RELAY_WINDOW2   17   // K2 — window motor 2
#define PIN_RELAY_PUMP      18   // K3 — solenoid valve 220V
#define PIN_RELAY_FAN1      19   // K4 — fan 1
#define PIN_RELAY_FAN2      23   // K5 — fan 2  (moved from 21: I2C needs 21/22)

// ── I2C bus — SHT31 temperature/humidity sensor ──────────────
#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22
#define SHT31_ADDR          0x44

// ── Analog soil sensor input (protected: RC + TVS on PCB) ────
#define PIN_SOIL_SENSOR     34   // ADC1_CH6 — input only

// ── RS485 port (MAX3485) for industrial Modbus sensors ───────
#define PIN_RS485_TX        25
#define PIN_RS485_RX        26
#define PIN_RS485_DE        27   // Driver-enable (TX direction control)

// ── Panel UI ─────────────────────────────────────────────────
#define PIN_LED_POWER       2    // Green  — power OK
#define PIN_LED_WIFI        4    // Blue   — WiFi connected (blink = connecting)
#define PIN_LED_STATUS      5    // Yellow — MQTT OK / pump running
#define PIN_BTN_BOOT        0    // BOOT: hold 5s = provisioning, 10s = factory reset

// ── Relay polarity: ULN2003A is ACTIVE-HIGH (sinks coil on HIGH input) ──────
#define RELAY_ACTIVE_LOW    false
#define RELAY_ON            (RELAY_ACTIVE_LOW ? LOW  : HIGH)
#define RELAY_OFF           (RELAY_ACTIVE_LOW ? HIGH : LOW)
