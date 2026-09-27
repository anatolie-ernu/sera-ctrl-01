#!/usr/bin/env bash
# =============================================================
# SERA INTELIGENTA — Setup Etapa 1
# Instalare automata mediu dezvoltare firmware ESP32
# Rulare: chmod +x setup.sh && sudo ./setup.sh
# =============================================================
set -euo pipefail
RED='\033[0;31m';GREEN='\033[0;32m';YELLOW='\033[1;33m';CYAN='\033[0;36m';NC='\033[0m'
log()     { echo -e "${GREEN}[✔] $*${NC}"; }
warn()    { echo -e "${YELLOW}[⚠] $*${NC}"; }
error()   { echo -e "${RED}[✘] $*${NC}"; exit 1; }
section() { echo -e "\n${CYAN}══════ $* ══════${NC}"; }

[[ "$OSTYPE" == "linux-gnu"* ]] && OS=linux || OS=macos
section "Setup Etapa 1 — OS: $OS"

if [[ "$OS" == "linux" ]]; then
    [[ $EUID -ne 0 ]] && error "Rulati cu: sudo ./setup.sh"
    apt-get update -qq
    apt-get install -y python3 python3-pip curl git usbutils screen 2>/dev/null
    log "Pachete Linux instalate"
else
    command -v brew &>/dev/null || \
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    brew install python3 git curl 2>/dev/null || true
    log "Pachete macOS instalate"
fi

section "PlatformIO CLI"
if command -v pio &>/dev/null; then
    log "PlatformIO deja instalat: $(pio --version)"
else
    pip3 install --upgrade platformio
    log "PlatformIO instalat: $(pio --version)"
fi

if [[ "$OS" == "linux" ]]; then
    section "Reguli udev ESP32"
    cat > /etc/udev/rules.d/99-esp32.rules << 'UDEV'
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", SYMLINK+="esp32", GROUP="dialout", MODE="0666"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="esp32", GROUP="dialout", MODE="0666"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4", SYMLINK+="esp32", GROUP="dialout", MODE="0666"
UDEV
    udevadm control --reload-rules && udevadm trigger
    REAL_USER="${SUDO_USER:-$USER}"
    usermod -aG dialout "$REAL_USER" 2>/dev/null || true
    log "Reguli udev configurate pentru user: $REAL_USER"
fi

section "Librarii firmware"
FW="$(dirname "$0")/../firmware"
if [[ -f "$FW/platformio.ini" ]]; then
    cd "$FW" && pio lib install && log "Librarii PlatformIO instalate"
else
    warn "platformio.ini negasit — librarii se instaleaza la primul build"
fi

section "Detectare port ESP32"
detect_port() {
    for p in /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyACM0 /dev/cu.usbserial-* /dev/cu.SLAB*; do
        [[ -e "$p" ]] && echo "$p" && return; done; echo "NOT_FOUND"
}
PORT=$(detect_port)
ENV_FILE="$(dirname "$0")/.env"
if [[ "$PORT" == "NOT_FOUND" ]]; then
    warn "ESP32 nedetectat. Conectati placa si rulati: ./detect_port.sh"
else
    log "ESP32 pe: $PORT"
    echo "ESP32_PORT=$PORT" > "$ENV_FILE"
fi

# Script detect_port
cat > "$(dirname "$0")/detect_port.sh" << 'DPS'
#!/usr/bin/env bash
echo "=== Porturi USB disponibile ===";
ls /dev/ttyUSB* /dev/ttyACM* /dev/cu.usb* /dev/cu.SLAB* 2>/dev/null || echo "Niciun port gasit"
DPS
chmod +x "$(dirname "$0")/detect_port.sh"

section "GATA!"
echo -e "${GREEN}Pasi urmatori:${NC}"
echo "  1. cd firmware/ && nano src/config.h   # seteaza pinii/pragurile"
echo "  2. ./scripts/flash.sh upload            # compileaza + upload"
echo "  3. ./scripts/flash.sh monitor           # Serial Monitor"
echo "  4. Scrie HELP in monitor pentru comenzi"
