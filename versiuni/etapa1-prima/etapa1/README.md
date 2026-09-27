# Etapa 1 — Firmware ESP32: Senzori + Actuatoare

## Conținut arhivă

```
etapa1_cod_sursa/
├── firmware/
│   ├── src/
│   │   ├── main.cpp              ← Punct de intrare principal
│   │   ├── config.h              ← ⚙️ CONFIGURARE - modifică pragurile aici
│   │   ├── sensors.h / .cpp      ← DHT22 + Senzor sol
│   │   ├── actuators.h / .cpp    ← Relee + Logică automată
│   │   ├── wifi_manager.h        ← Placeholder (Etapa 2)
│   │   └── display.h             ← Placeholder (Etapa 3)
│   ├── platformio.ini            ← Configurare PlatformIO/Arduino IDE
│   └── schema_conexiuni.txt      ← Schema cablare ASCII
└── docs/
    ├── DOCUMENTATIE_ETAPA1.md    ← Documentație completă
    └── PLAN_COMPLET_10_ETAPE.md  ← Planul tuturor etapelor
```

## Start rapid

1. Instalează [VS Code](https://code.visualstudio.com/) + extensia PlatformIO
2. Deschide folderul `firmware/` în VS Code
3. Editează `src/config.h` pentru a seta pinii și pragurile tale
4. Click **Upload** (sau `Ctrl+Alt+U`)
5. Deschide **Serial Monitor** la 115200 baud
6. Tastează `HELP` pentru a vedea comenzile disponibile

## Pași importanți de calibrare

Înainte de utilizare, calibrează senzorul de sol:
- În aer (uscat) → notează valoarea ADC → setează `ADC_DRY` în `sensors.cpp`
- În apă (ud) → notează valoarea ADC → setează `ADC_WET` în `sensors.cpp`

## Suport

Vezi `docs/DOCUMENTATIE_ETAPA1.md` pentru documentație completă inclusiv
schema de cablare, calibrare, testare și troubleshooting.
