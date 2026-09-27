#!/usr/bin/env bash
# run_tests.sh — Ruleaza testele E2E impotriva unui server pornit
set -euo pipefail
GREEN='\033[0;32m';CYAN='\033[0;36m';NC='\033[0m'
API="${1:-http://localhost:3000}"
echo -e "${CYAN}=== Teste E2E impotriva: $API ===${NC}"
cd "$(dirname "$0")/.."
npm install --silent
API_URL="$API" npm test
echo -e "${GREEN}✔ Toate testele trecute!${NC}"
