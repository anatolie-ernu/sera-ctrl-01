#!/usr/bin/env bash
set -euo pipefail
GREEN='\033[0;32m';CYAN='\033[0;36m';NC='\033[0m'
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT"
[[ ! -f .env ]] && echo "Lipseste .env! cp .env.example .env si completeaza." && exit 1
echo -e "${CYAN}=== Pornire Sera Inteligenta ===${NC}"
docker compose pull --quiet 2>/dev/null || true
docker compose up -d --build
echo -e "${GREEN}✔ Servicii pornite:${NC}"
docker compose ps
echo ""
echo -e "${CYAN}Logs: docker compose logs -f backend${NC}"
echo -e "${CYAN}Stop: ./scripts/stop.sh${NC}"
