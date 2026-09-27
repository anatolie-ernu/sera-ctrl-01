#!/usr/bin/env bash
# ssl.sh — Instalare certificat SSL Let's Encrypt
set -euo pipefail
GREEN='\033[0;32m';CYAN='\033[0;36m';NC='\033[0m'
cd "$(dirname "$0")/.."
source .env
echo -e "${CYAN}=== Instalare SSL pentru $DOMAIN ===${NC}"
# Certbot initial (doar HTTP challenge)
docker compose run --rm certbot certonly \
    --webroot -w /var/www/certbot \
    -d "$DOMAIN" \
    --email "$CERTBOT_EMAIL" \
    --agree-tos --no-eff-email --force-renewal
docker compose exec nginx nginx -s reload
echo -e "${GREEN}✔ SSL instalat pentru $DOMAIN${NC}"
echo "Auto-renew configurat (la fiecare 12h)"
