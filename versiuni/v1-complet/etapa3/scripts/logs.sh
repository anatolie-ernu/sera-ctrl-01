#!/usr/bin/env bash
cd "$(dirname "$0")/.."
SERVICE="${1:-backend}"
docker compose logs -f "$SERVICE"
