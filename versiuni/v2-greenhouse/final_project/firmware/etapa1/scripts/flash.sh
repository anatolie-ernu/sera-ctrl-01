#!/usr/bin/env bash
# flash.sh — Compilare, upload, monitorizare ESP32
# Utilizare: ./flash.sh [upload|monitor|all|clean]
set -euo pipefail
RED='\033[0;31m';GREEN='\033[0;32m';CYAN='\033[0;36m';NC='\033[0m'
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FW="$SCRIPT_DIR/../firmware"
[[ -f "$SCRIPT_DIR/.env" ]] && source "$SCRIPT_DIR/.env"
PORT="${ESP32_PORT:-}"
[[ -z "$PORT" ]] && for p in /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyACM0 /dev/cu.usbserial-0001; do
    [[ -e "$p" ]] && PORT="$p" && break; done

upload()  {
    echo -e "${GREEN}=== Upload firmware ===${NC}"
    cd "$FW"
    [[ -n "$PORT" ]] && pio run -t upload --upload-port "$PORT" || pio run -t upload
    echo -e "${GREEN}✔ Upload OK${NC}"
}
monitor() {
    echo -e "${GREEN}=== Serial Monitor (Ctrl+C iesire) ===${NC}"
    cd "$FW"
    [[ -n "$PORT" ]] && pio device monitor --port "$PORT" --baud 115200 --filter colorize \
        || pio device monitor --baud 115200 --filter colorize
}
case "${1:-all}" in
    upload)  upload ;;
    monitor) monitor ;;
    all)     upload && sleep 2 && monitor ;;
    clean)   cd "$FW" && pio run -t clean ;;
    *)       echo "Utilizare: $0 [upload|monitor|all|clean]"; exit 1 ;;
esac
