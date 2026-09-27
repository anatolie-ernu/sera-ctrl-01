#!/usr/bin/env bash
# Acces direct la psql in container
cd "$(dirname "$0")/.."
source .env
docker compose exec timescaledb psql -U "$POSTGRES_USER" -d "$POSTGRES_DB"
