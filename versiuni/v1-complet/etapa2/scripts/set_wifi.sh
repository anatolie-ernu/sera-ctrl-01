#!/usr/bin/env bash
# set_wifi.sh — Seteaza WiFi+MQTT in config.h fara editare manuala
set -euo pipefail
GREEN='\033[0;32m';CYAN='\033[0;36m';NC='\033[0m'
CONFIG="$(dirname "$0")/../firmware/src/config.h"
[[ ! -f "$CONFIG" ]] && echo "config.h negasit!" && exit 1
echo -e "${CYAN}=== Configurare WiFi + MQTT ===${NC}"
read -p "WiFi SSID: " SSID
read -s -p "WiFi Parola: " WPASS; echo
read -p "IP Server MQTT (ex: 192.168.1.100): " MHOST
read -p "MQTT User [sera_device]: " MUSER; MUSER="${MUSER:-sera_device}"
read -s -p "MQTT Parola: " MPASS; echo
sed -i.bak \
    -e "s|#define WIFI_SSID.*|#define WIFI_SSID          \"$SSID\"|" \
    -e "s|#define WIFI_PASSWORD.*|#define WIFI_PASSWORD      \"$WPASS\"|" \
    -e "s|#define MQTT_HOST.*|#define MQTT_HOST          \"$MHOST\"|" \
    -e "s|#define MQTT_USER.*|#define MQTT_USER          \"$MUSER\"|" \
    -e "s|#define MQTT_PASS.*|#define MQTT_PASS          \"$MPASS\"|" \
    "$CONFIG"
echo -e "${GREEN}✔ config.h actualizat. Ruleaza: ./flash.sh upload${NC}"
