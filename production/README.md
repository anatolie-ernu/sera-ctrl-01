# Linie de producție SERA-CTRL-01

## PKI — Certificate per dispozitiv
```bash
# 1. Configurați Root CA (o singură dată, offline)
cd ca && bash ca_setup.sh

# 2. Generați certificat pentru fiecare dispozitiv
bash gen_device_cert.sh SERA-A1B2C3
```

## Jig de testare EOL
```bash
cd jig
pip install pyserial cryptography reportlab qrcode
python jig_test.py --port /dev/ttyUSB0 --device-id SERA-A1B2C3
```
Testul durează < 90s/unitate și produce:
- Raport PDF cu rezultate
- Etichetă QR pentru produs
- Înregistrare în baza de date SQLite locală
