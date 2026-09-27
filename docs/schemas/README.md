# Scheme Electrice SERA-CTRL-01

## E01 — Schema IEC Principală (12V DC + UPS Solar)
Schemă inginerească clasică IEC 60617 cu:
- TR1 230V/15V 200VA → punte Graetz → filtru → +12V
- Panou solar 100W → MPPT 10A
- D6/D7 diode separare Schottky 20A
- Acumulator 12V 60Ah AGM/LiFePO4
- M1/M2 motoare 12V 5A, electrovalvă, ventilatoare
- Chenar titlu, BOM 22 componente, 18 note tehnice

## E02 — Wiring Diagram 12V DC + UPS Solar
Schemă de cablare clară (stil inventable.eu) cu:
- Zone rectangulare fixe per bloc (AC, Solar, MPPT, Bat, Sarcini, Control)
- Text niciodată pe fire — coridoare dedicate
- K1-K5 relee 12V cu pini C/NO/NC clar etichetați
- ESP32 + ULN2003A + buzzer alertă baterie
