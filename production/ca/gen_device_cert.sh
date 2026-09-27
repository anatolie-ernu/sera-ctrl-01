#!/usr/bin/env bash
# =============================================================================
# gen_device_cert.sh — Emite certificat X.509 unic pentru un dispozitiv
# =============================================================================
# Apelat automat de jig_test.py la fiecare unitate care trece testul.
# Utilizare manuală:  ./gen_device_cert.sh SERA-A1B2C3
#
# CN-ul certificatului = device_id → brokerul MQTT mapează identitatea
# (use_identity_as_username true) și ACL-ul restrânge dispozitivul la
# topicurile lui: greenhouse/<CN>/#
# =============================================================================
set -euo pipefail
DEVICE_ID="${1:?Utilizare: $0 <DEVICE_ID>}"
cd "$(dirname "$0")/ca"
OUT="devices/${DEVICE_ID}"
mkdir -p "$OUT"

# Cheie + CSR + certificat semnat de Device CA (valabil 10 ani)
openssl genrsa -out "$OUT/device.key" 2048
openssl req -new -key "$OUT/device.key" \
    -subj "/O=Sera Inteligenta/CN=${DEVICE_ID}" -out "$OUT/device.csr"
openssl x509 -req -in "$OUT/device.csr" \
    -CA device-ca/device-ca.crt -CAkey device-ca/device-ca.key -CAcreateserial \
    -days 3650 -sha256 \
    -extfile <(echo "basicConstraints=CA:FALSE
keyUsage=digitalSignature,keyEncipherment
extendedKeyUsage=clientAuth") \
    -out "$OUT/device.crt"
rm "$OUT/device.csr"

echo "[CERT] ${DEVICE_ID}: $OUT/device.crt + device.key"
echo "[CERT] Acestea se scriu în NVS-ul dispozitivului la programare (criptate de Flash Encryption)"
