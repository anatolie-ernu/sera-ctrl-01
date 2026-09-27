#!/usr/bin/env bash
# calibrate_soil.sh — Ghid calibrare senzor sol prin Serial Monitor
set -euo pipefail
CYAN='\033[0;36m';GREEN='\033[0;32m';YELLOW='\033[1;33m';NC='\033[0m'
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
[[ -f "$SCRIPT_DIR/.env" ]] && source "$SCRIPT_DIR/.env"
PORT="${ESP32_PORT:-/dev/ttyUSB0}"

echo -e "${CYAN}=== Calibrare Senzor Sol ===${NC}"
echo -e "${YELLOW}Pasul 1:${NC} Pune senzorul in AER (simulare sol uscat)"
echo "Asteapta 5 secunde si citeste valoarea ADC din monitor"
read -p "Introdu valoarea ADC in aer (ex: 3150): " ADC_DRY

echo -e "${YELLOW}Pasul 2:${NC} Pune senzorul in PAHAR CU APA (simulare sol ud)"
echo "Asteapta 5 secunde si citeste valoarea ADC din monitor"
read -p "Introdu valoarea ADC in apa (ex: 820): " ADC_WET

CONFIG_FILE="$SCRIPT_DIR/../firmware/src/config.h"
if [[ -f "$CONFIG_FILE" ]]; then
    sed -i.bak "s/#define SOIL_ADC_DRY.*/#define SOIL_ADC_DRY         $ADC_DRY/" "$CONFIG_FILE"
    sed -i.bak "s/#define SOIL_ADC_WET.*/#define SOIL_ADC_WET          $ADC_WET/" "$CONFIG_FILE"
    echo -e "${GREEN}✔ config.h actualizat: DRY=$ADC_DRY WET=$ADC_WET${NC}"
    echo "Refa upload: ./flash.sh upload"
else
    echo -e "${YELLOW}config.h negasit. Seteaza manual:${NC}"
    echo "  #define SOIL_ADC_DRY    $ADC_DRY"
    echo "  #define SOIL_ADC_WET    $ADC_WET"
fi
