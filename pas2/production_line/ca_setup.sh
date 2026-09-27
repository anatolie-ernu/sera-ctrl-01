#!/usr/bin/env bash
# =============================================================================
# ca_setup.sh — Creează CA-ul companiei pentru certificate per dispozitiv
# =============================================================================
# RULAȚI O SINGURĂ DATĂ, pe o mașină OFFLINE (air-gapped)!
# Cheia root NU părăsește niciodată această mașină. Pe linia de producție
# se copiază DOAR intermediarul (device-ca) + lanțul public.
#
# Structura generată:
#   ca/root/      — Root CA (valabil 20 ani, păstrat offline, 2 copii USB)
#   ca/device-ca/ — Intermediate CA pentru dispozitive (valabil 10 ani)
#   ca/chain.pem  — lanțul public (se instalează pe broker + în firmware)
# =============================================================================
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p ca/root ca/device-ca && cd ca

ORG="Sera Inteligenta SRL"   # ← schimbați cu denumirea firmei

# ── 1. Root CA ───────────────────────────────────────────────────────────────
if [[ ! -f root/root.key ]]; then
    openssl genrsa -aes256 -out root/root.key 4096       # cere parolă — notați-o în seif!
    openssl req -x509 -new -key root/root.key -sha256 -days 7300 \
        -subj "/C=MD/O=${ORG}/CN=${ORG} Root CA" \
        -out root/root.crt
    echo "[CA] Root CA creat (valabil 20 ani)"
fi

# ── 2. Intermediate CA pentru dispozitive ────────────────────────────────────
if [[ ! -f device-ca/device-ca.key ]]; then
    openssl genrsa -out device-ca/device-ca.key 3072
    openssl req -new -key device-ca/device-ca.key \
        -subj "/C=MD/O=${ORG}/CN=${ORG} Device CA" \
        -out device-ca/device-ca.csr
    # pathlen:0 = intermediarul NU poate emite alte CA-uri (limitare daune)
    openssl x509 -req -in device-ca/device-ca.csr \
        -CA root/root.crt -CAkey root/root.key -CAcreateserial \
        -days 3650 -sha256 \
        -extfile <(echo "basicConstraints=critical,CA:TRUE,pathlen:0
keyUsage=critical,keyCertSign,cRLSign") \
        -out device-ca/device-ca.crt
    rm device-ca/device-ca.csr
    echo "[CA] Device CA creat (valabil 10 ani)"
fi

# ── 3. Lanțul public ─────────────────────────────────────────────────────────
cat device-ca/device-ca.crt root/root.crt > chain.pem
echo "[CA] chain.pem generat — instalați pe broker (cafile) și în firmware"
echo ""
echo "COPIAȚI pe linia de producție DOAR: device-ca/ + chain.pem"
echo "Root key rămâne OFFLINE. Faceți 2 copii USB ale întregului director ca/."
