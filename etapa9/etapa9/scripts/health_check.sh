#!/usr/bin/env bash
# health_check.sh — Verifica starea tuturor serviciilor
set -euo pipefail
RED='\033[0;31m';GREEN='\033[0;32m';YELLOW='\033[1;33m';NC='\033[0m'
ok()   { echo -e "${GREEN}[OK]  $*${NC}"; }
fail() { echo -e "${RED}[ERR] $*${NC}"; ERRORS=$((ERRORS+1)); }
warn() { echo -e "${YELLOW}[WRN] $*${NC}"; }
ERRORS=0

cd /opt/greenhouse
source .env 2>/dev/null || true

echo "=== Health Check Sera Inteligenta $(date) ==="

# Docker services
for svc in sera_tsdb sera_mqtt sera_redis sera_backend sera_nginx; do
    STATUS=$(docker inspect --format='{{.State.Status}}' "$svc" 2>/dev/null || echo "missing")
    [[ "$STATUS" == "running" ]] && ok "Docker: $svc" || fail "Docker: $svc ($STATUS)"
done

# Backend API
HTTP=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:3000/health 2>/dev/null || echo "000")
[[ "$HTTP" == "200" ]] && ok "API: /health HTTP $HTTP" || fail "API: /health HTTP $HTTP"

# MQTT
nc -z localhost 1883 2>/dev/null && ok "MQTT: port 1883 deschis" || fail "MQTT: port 1883 inaccesibil"

# TimescaleDB
docker compose exec -T timescaledb pg_isready -U "$POSTGRES_USER" -q 2>/dev/null \
    && ok "TimescaleDB: ready" || fail "TimescaleDB: not ready"

# Disk space
DISK=$(df / | awk 'NR==2{print $5}' | tr -d '%')
[[ $DISK -lt 80 ]] && ok "Disk: ${DISK}% utilizat" || warn "Disk: ${DISK}% utilizat (>80%!)"

# RAM
MEM=$(free | awk '/Mem/{printf "%.0f", $3/$2*100}')
[[ $MEM -lt 90 ]] && ok "RAM: ${MEM}% utilizat" || warn "RAM: ${MEM}% utilizat (>90%!)"

echo ""
[[ $ERRORS -eq 0 ]] \
    && echo -e "${GREEN}✔ Toate verificarile trecute!${NC}" \
    || echo -e "${RED}✘ $ERRORS erori detectate!${NC}"
exit $ERRORS
