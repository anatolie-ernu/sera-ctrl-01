#!/usr/bin/env bash
# =============================================================
# install_server.sh — Instalare completa server Linux
# Testat pe: Ubuntu 22.04 / 24.04, Debian 12
# Rulare: chmod +x install_server.sh && sudo ./install_server.sh
# =============================================================
set -euo pipefail
RED='\033[0;31m';GREEN='\033[0;32m';YELLOW='\033[1;33m';CYAN='\033[0;36m';NC='\033[0m'
log()     { echo -e "${GREEN}[✔] $*${NC}"; }
warn()    { echo -e "${YELLOW}[⚠] $*${NC}"; }
section() { echo -e "\n${CYAN}══════ $* ══════${NC}"; }

[[ $EUID -ne 0 ]] && echo "Rulati cu: sudo ./install_server.sh" && exit 1

section "Update sistem"
apt-get update -qq && apt-get upgrade -y -qq
apt-get install -y curl wget git nano htop ufw fail2ban \
    ca-certificates gnupg lsb-release 2>/dev/null
log "Pachete sistem instalate"

section "Instalare Docker"
if command -v docker &>/dev/null; then
    log "Docker deja instalat: $(docker --version)"
else
    curl -fsSL https://get.docker.com | sh
    REAL_USER="${SUDO_USER:-$USER}"
    usermod -aG docker "$REAL_USER"
    systemctl enable docker && systemctl start docker
    log "Docker instalat: $(docker --version)"
fi

section "Instalare Docker Compose v2"
if docker compose version &>/dev/null; then
    log "Docker Compose deja instalat: $(docker compose version)"
else
    COMPOSE_VER=$(curl -s https://api.github.com/repos/docker/compose/releases/latest|grep tag_name|cut -d'"' -f4)
    curl -SL "https://github.com/docker/compose/releases/download/${COMPOSE_VER}/docker-compose-linux-x86_64" \
        -o /usr/local/lib/docker/cli-plugins/docker-compose
    chmod +x /usr/local/lib/docker/cli-plugins/docker-compose
    log "Docker Compose instalat: $(docker compose version)"
fi

section "Firewall UFW"
ufw --force reset
ufw default deny incoming
ufw default allow outgoing
ufw allow ssh
ufw allow 80/tcp comment 'HTTP'
ufw allow 443/tcp comment 'HTTPS'
ufw allow 1883/tcp comment 'MQTT (LAN only - restrictionati prin IP!)'
ufw --force enable
log "Firewall configurat"

section "Fail2ban"
systemctl enable fail2ban && systemctl start fail2ban
log "Fail2ban activ"

section "Creare structura proiect"
PROJECT_DIR="/opt/greenhouse"
mkdir -p "$PROJECT_DIR"
cp -r "$(dirname "$0")/../"* "$PROJECT_DIR/" 2>/dev/null || true
cd "$PROJECT_DIR"

if [[ ! -f .env ]]; then
    cp .env.example .env
    warn "Fisier .env creat din .env.example"
    warn "EDITATI .env inainte de a continua: nano .env"
    warn "Apoi rulati: ./scripts/start.sh"
else
    log ".env deja configurat"
fi

# Genereaza JWT secrets automat
if grep -q "genereaza_cu_openssl" .env 2>/dev/null; then
    JWT_S=$(openssl rand -hex 64)
    JWT_R=$(openssl rand -hex 64)
    sed -i "s|genereaza_cu_openssl_rand_hex_64_aici|$JWT_S|" .env
    sed -i "s|genereaza_alt_secret_diferit_hex_64|$JWT_R|" .env
    log "JWT secrets generate automat"
fi

section "Configurare MQTT passwords"
MQTT_PASS_FILE="$PROJECT_DIR/docker/mosquitto/config/passwd"
if [[ ! -f "$MQTT_PASS_FILE" ]]; then
    source .env 2>/dev/null || true
    docker run --rm eclipse-mosquitto:2 mosquitto_passwd -b /dev/stdout \
        "${MQTT_USER:-sera_backend}" "${MQTT_PASS:-changeme}" > "$MQTT_PASS_FILE" 2>/dev/null
    docker run --rm eclipse-mosquitto:2 mosquitto_passwd -b /dev/stdout \
        "sera_device" "device_pass" >> "$MQTT_PASS_FILE" 2>/dev/null
    log "MQTT passwords create"
fi

section "Systemd service (auto-start)"
cat > /etc/systemd/system/greenhouse.service << SERVICE
[Unit]
Description=Sera Inteligenta
After=docker.service
Requires=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=$PROJECT_DIR
ExecStart=/usr/local/lib/docker/cli-plugins/docker-compose up -d
ExecStop=/usr/local/lib/docker/cli-plugins/docker-compose down
Restart=on-failure

[Install]
WantedBy=multi-user.target
SERVICE
systemctl daemon-reload
systemctl enable greenhouse
log "Serviciu systemd creat si activat"

section "INSTALARE COMPLETA!"
echo ""
echo -e "${GREEN}Pasi urmatori:${NC}"
echo "  1. nano $PROJECT_DIR/.env              # completeaza variabilele"
echo "  2. cd $PROJECT_DIR && ./scripts/start.sh  # porneste serviciile"
echo "  3. ./scripts/ssl.sh                    # instaleaza SSL (dupa DNS configurat)"
echo ""
echo -e "${YELLOW}Apoi accesati: https://\$DOMAIN${NC}"
