# Firmware SERA-CTRL-01

## Etapa 1 — Standalone (fără WiFi)
Funcționare autonomă: citire senzori, control relee pe praguri locale.
Ideal pentru testare hardware fără infrastructură.
```bash
cd etapa1 && pio run --target upload
```

## Etapa 2 — WiFi + MQTT
Conectare la broker Mosquitto, publicare senzori, primire comenzi.
```bash
cd etapa2 && pio run --target upload
```

## Production — Firmware complet
NVS pentru config persistentă, portal captiv SoftAP, OTA HTTPS,
Secure Boot v2, factory test automat la EOL.
Pin map: `production/src/hardware.h`
