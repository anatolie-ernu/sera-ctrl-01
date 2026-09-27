#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
=============================================================================
jig_test.py — Stația de test End-Of-Line | SERA-CTRL-01
=============================================================================
Rulează pe PC-ul jigului de test (Linux, Raspberry Pi este suficient).
Operatorul așază placa pe pat, apasă START (Enter) — restul e automat.

FLUX COMPLET PER UNITATE (țintă < 90 secunde):
  1. Detectare port USB (CP2102N enumerat)
  2. Flash firmware de producție (esptool, imagine semnată)
  3. Reset → trimite FACTORY_TEST pe serial
  4. Parsează linia TEST_RESULT;<device_id>;PASS/FAIL;<detalii>
  5. La PASS:
       a. Emite certificat X.509 unic (gen_device_cert.sh)
       b. Înregistrează unitatea în baza de producție (SQLite local,
          sincronizată ulterior cu serverul)
       c. Generează cod de claim (tipărit ca QR pe etichetă)
       d. Generează eticheta produsului (PDF 50×30 mm: serie, QR, CE, MAC)
  6. La FAIL: înregistrează defectul, placa merge la diagnoză

INSTALARE (o singură dată pe PC-ul jigului):
  pip install esptool pyserial qrcode reportlab
  ./ca_setup.sh trebuie rulat în prealabil (sau copiat device-ca/ aici)

UTILIZARE:
  python3 jig_test.py                    # mod interactiv (operator)
  python3 jig_test.py --port /dev/ttyUSB0 --no-flash   # doar re-test
=============================================================================
"""
import argparse
import datetime
import glob
import re
import secrets
import sqlite3
import subprocess
import sys
import time
from pathlib import Path

import serial  # pyserial

# ── Configurare stație ───────────────────────────────────────────────────────
FIRMWARE_BIN   = Path("firmware/sera_v3_signed.bin")   # imaginea semnată de CI
BOOTLOADER_BIN = Path("firmware/bootloader_signed.bin")
PARTITIONS_BIN = Path("firmware/partition-table.bin")
OTADATA_BIN    = Path("firmware/ota_data_initial.bin")
DB_PATH        = Path("production.db")
LABEL_DIR      = Path("labels")
BAUD           = 115200
SERIAL_TIMEOUT = 3      # secunde per citire
TEST_TIMEOUT   = 60     # secunde maxim pentru întreaga secvență de test

# ─────────────────────────────────────────────────────────────────────────────
# BAZA DE DATE DE PRODUCȚIE
# Fiecare unitate testată = un rând. Tabelul este sursa trasabilității:
# la un RMA, după serie afli lotul PCB, versiunea FW și rezultatul testului.
# ─────────────────────────────────────────────────────────────────────────────
def db_init():
    con = sqlite3.connect(DB_PATH)
    con.execute("""
        CREATE TABLE IF NOT EXISTS production_units (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id   TEXT UNIQUE,         -- SERA-XXXXXX (din MAC)
            tested_at   TEXT,                -- ISO 8601
            result      TEXT,                -- PASS / FAIL
            fail_detail TEXT,                -- module picate, separate cu ;
            fw_version  TEXT,
            pcb_lot     TEXT,                -- lotul PCB (introdus de operator la început de schimb)
            claim_code  TEXT,                -- codul QR de pe etichetă (clientul îl scanează)
            operator    TEXT,
            synced      INTEGER DEFAULT 0    -- 1 după sincronizarea cu serverul central
        )""")
    con.commit()
    return con

# ─────────────────────────────────────────────────────────────────────────────
def find_port() -> str:
    """Detectează portul serial al plăcii (CP2102N → /dev/ttyUSB*)."""
    for _ in range(20):                      # așteaptă max 10 s enumerarea USB
        ports = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
        if ports:
            return ports[0]
        time.sleep(0.5)
    raise RuntimeError("Placa nu a fost detectată pe USB. Verificați pogo-pins.")

# ─────────────────────────────────────────────────────────────────────────────
def flash_firmware(port: str):
    """Scrie imaginea de producție cu esptool. Ridică excepție la eroare."""
    print("  [2/6] Flash firmware...", flush=True)
    cmd = [
        sys.executable, "-m", "esptool",
        "--chip", "esp32", "--port", port, "--baud", "921600",
        "write_flash",
        "0x1000",  str(BOOTLOADER_BIN),
        "0x8000",  str(PARTITIONS_BIN),
        "0xe000",  str(OTADATA_BIN),
        "0x10000", str(FIRMWARE_BIN),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=180)
    if r.returncode != 0:
        raise RuntimeError(f"esptool a eșuat:\n{r.stderr[-800:]}")
    print("        flash OK")

# ─────────────────────────────────────────────────────────────────────────────
def run_factory_test(port: str) -> tuple[str, str, str]:
    """
    Deschide serialul, trimite FACTORY_TEST în fereastra de 5 s de la boot
    și așteaptă linia TEST_RESULT;<id>;<PASS|FAIL>;<detalii>.
    @return (device_id, result, detail)
    """
    print("  [3/6] Rulare factory test...", flush=True)
    with serial.Serial(port, BAUD, timeout=SERIAL_TIMEOUT) as ser:
        # Reset hardware prin DTR/RTS (același mecanism ca esptool/IDE)
        ser.dtr = False; ser.rts = True;  time.sleep(0.1)
        ser.rts = False;                  time.sleep(0.3)

        # Trimite comanda repetat în fereastra de boot (firmware ascultă 5 s)
        deadline = time.time() + TEST_TIMEOUT
        sent = 0
        while time.time() < deadline:
            if sent < 10:
                ser.write(b"FACTORY_TEST\n")
                sent += 1
            line = ser.readline().decode(errors="replace").strip()
            if line:
                print(f"        | {line}")
            m = re.match(r"TEST_RESULT;([^;]+);(PASS|FAIL);(.*)", line)
            if m:
                return m.group(1), m.group(2), m.group(3)
    raise RuntimeError("Timeout: placa nu a raportat TEST_RESULT")

# ─────────────────────────────────────────────────────────────────────────────
def issue_certificate(device_id: str) -> Path:
    """Emite certificatul X.509 al dispozitivului prin scriptul CA."""
    print("  [4/6] Emitere certificat dispozitiv...", flush=True)
    r = subprocess.run(["./gen_device_cert.sh", device_id],
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"Emitere certificat eșuată:\n{r.stderr}")
    cert_dir = Path("ca/devices") / device_id
    print(f"        cert: {cert_dir}/device.crt")
    # NOTĂ: scrierea cert+key în NVS-ul plăcii se face cu nvs_partition_gen
    # + esptool write_flash pe partiția nvs — pas adăugat la activarea MQTTS.
    return cert_dir

# ─────────────────────────────────────────────────────────────────────────────
def make_label(device_id: str, claim_code: str) -> Path:
    """
    Generează eticheta produsului ca PDF 50×30 mm:
      - device_id (serie) text + QR cu payload-ul de claim
      - marcaj CE, model, tensiune (cerințe etichetare LVD)
    PDF-ul se trimite la imprimanta de etichete (Brother QL / Zebra cu driver CUPS).
    """
    print("  [5/6] Generare etichetă...", flush=True)
    import qrcode
    from reportlab.lib.units import mm
    from reportlab.pdfgen import canvas

    LABEL_DIR.mkdir(exist_ok=True)
    qr_payload = f"https://app.sera.md/claim?d={device_id}&c={claim_code}"
    qr_path = LABEL_DIR / f"{device_id}.png"
    qrcode.make(qr_payload).save(qr_path)

    pdf_path = LABEL_DIR / f"{device_id}.pdf"
    c = canvas.Canvas(str(pdf_path), pagesize=(50 * mm, 30 * mm))
    c.drawImage(str(qr_path), 2 * mm, 3 * mm, 24 * mm, 24 * mm)
    c.setFont("Helvetica-Bold", 8)
    c.drawString(28 * mm, 23 * mm, "SERA-CTRL-01")
    c.setFont("Helvetica", 7)
    c.drawString(28 * mm, 18 * mm, device_id)
    c.drawString(28 * mm, 13 * mm, "230V~ 50Hz  T2A")
    c.setFont("Helvetica-Bold", 10)
    c.drawString(28 * mm, 6 * mm, "CE")          # + WEEE/producător pe ambalaj
    c.save()
    print(f"        etichetă: {pdf_path}")
    # Tipărire automată dacă imprimanta e configurată în CUPS:
    # subprocess.run(["lp", "-d", "labelprinter", str(pdf_path)])
    return pdf_path

# ─────────────────────────────────────────────────────────────────────────────
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", help="Port serial (implicit: autodetect)")
    ap.add_argument("--no-flash", action="store_true", help="Sari peste flash (re-test)")
    ap.add_argument("--lot", default="", help="Lot PCB (ex: 2026-W24-A)")
    ap.add_argument("--operator", default="op1")
    args = ap.parse_args()

    con = db_init()
    lot = args.lot or input("Lot PCB pentru acest schimb: ").strip()

    print("\n=== STAȚIE TEST SERA-CTRL-01 — gata. Ctrl+C pentru oprire. ===")
    unit_no = 0
    while True:
        input(f"\n[{unit_no+1}] Așezați placa pe jig și apăsați ENTER...")
        t0 = time.time()
        try:
            port = args.port or find_port()
            print(f"  [1/6] Port: {port}")

            if not args.no_flash:
                flash_firmware(port)

            device_id, result, detail = run_factory_test(port)

            claim_code = ""
            if result == "PASS":
                issue_certificate(device_id)
                claim_code = secrets.token_hex(4).upper()   # 8 caractere, pe etichetă
                make_label(device_id, claim_code)

            # [6/6] Înregistrare în baza de producție
            con.execute("""INSERT OR REPLACE INTO production_units
                (device_id, tested_at, result, fail_detail,
                 fw_version, pcb_lot, claim_code, operator)
                VALUES (?,?,?,?,?,?,?,?)""",
                (device_id, datetime.datetime.now().isoformat(timespec="seconds"),
                 result, detail if result == "FAIL" else "",
                 "3.0.0", lot, claim_code, args.operator))
            con.commit()

            dt = time.time() - t0
            verdict = "✅ PASS" if result == "PASS" else f"❌ FAIL ({detail})"
            print(f"  [6/6] {device_id}: {verdict}  ({dt:.0f}s)")
            unit_no += 1

        except KeyboardInterrupt:
            raise
        except Exception as e:
            print(f"  ✘ EROARE STAȚIE: {e}")
            print("    Placa merge la diagnoză. Continuați cu următoarea.")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStație oprită. Sumar zi:")
        con = sqlite3.connect(DB_PATH)
        for row in con.execute("""SELECT result, COUNT(*) FROM production_units
                WHERE date(tested_at)=date('now') GROUP BY result"""):
            print(f"  {row[0]}: {row[1]} unități")
