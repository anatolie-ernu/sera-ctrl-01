#!/usr/bin/env bash
# Adauga user nou in Mosquitto
USER="${1:-nouser}"; PASS="${2:-nopass}"
[[ "$USER" == "nouser" ]] && echo "Utilizare: $0 <user> <parola>" && exit 1
cd "$(dirname "$0")/.."
docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/passwd "$USER" "$PASS"
docker compose restart mosquitto
echo "User MQTT '$USER' adaugat."
