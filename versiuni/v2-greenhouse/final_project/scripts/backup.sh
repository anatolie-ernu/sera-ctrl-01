#!/usr/bin/env bash
# backup.sh — Backup TimescaleDB in fisier comprimat
set -euo pipefail
GREEN='\033[0;32m';NC='\033[0m'
BACKUP_DIR="${BACKUP_DIR:-/opt/greenhouse/backups}"
DATE=$(date +%Y%m%d_%H%M%S)
FILE="$BACKUP_DIR/sera_db_$DATE.sql.gz"
mkdir -p "$BACKUP_DIR"
cd "$(dirname "$0")/.."
source .env 2>/dev/null || true
echo "Backup TimescaleDB -> $FILE"
docker compose exec -T timescaledb pg_dump \
    -U "${POSTGRES_USER:-sera_user}" "${POSTGRES_DB:-sera_db}" \
    | gzip > "$FILE"
echo -e "${GREEN}✔ Backup creat: $FILE ($(du -sh "$FILE"|cut -f1))${NC}"
# Sterge backup-uri mai vechi de 30 zile
find "$BACKUP_DIR" -name "sera_db_*.sql.gz" -mtime +30 -delete
echo "Backup-uri vechi curatate."
