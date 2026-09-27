#!/usr/bin/env bash
cd "$(dirname "$0")/.."
echo "Oprire servicii..."
docker compose down
echo "Oprit."
