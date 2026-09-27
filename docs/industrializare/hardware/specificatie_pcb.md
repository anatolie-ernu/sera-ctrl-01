# SERA-CTRL-01 rev A — Specificație Fabricație PCB
## Document de trimis fabricantului (JLCPCB / PCBWay / Eurocircuits)

## Parametri placă
| Parametru | Valoare | Justificare |
|-----------|---------|-------------|
| Dimensiuni | 130 × 90 mm | Încape în carcasă DIN 9 module sau IP65 150×110 |
| Straturi | **2 straturi** | Suficient; 4 straturi doar dacă EMC-ul pică la teste |
| Material | FR-4 TG150 | Standard, temperatura din carcasă < 70°C |
| Grosime | 1.6 mm | Standard |
| Cupru | **2 oz (70µm)** exterior | Necesar pentru 12A pe contactele releu |
| Finisaj | HASL lead-free sau ENIG | ENIG dacă montați la EMS cu stencil fin |
| Mască | Verde, serigrafie albă | + marcaj "230V~ PERICOL" în zona AC |
| Min track/gap | 0.2/0.2 mm zona logică | Zona 230V: vezi mai jos |

## Reguli critice de layout

### Separare 230V / SELV (cea mai importantă regulă!)
- **Creepage minim 6.4 mm** între orice traseu/pad 230V și orice traseu SELV
  (IEC 62368-1, 250V working, poluare gradul 2, material grup IIIa)
- **Sloturi frezate 2 mm lățime** sub PS1 (între primar/secundar) și între
  pinii bobină/contact ai fiecărui releu HF115F
- Toate componentele 230V grupate într-o singură zonă a plăcii (stânga),
  logica în zona opusă (dreapta), senzorii pe muchia de jos

### Trasee de putere
- Contacte releu (COM/NO): lățime ≥ 3 mm la 2oz pentru 12A
- Întărire cu strat de cositor expus (mască deschisă) pe traseele de curent mare
- 5V de la PS1 la relee: ≥ 1.5 mm (5 bobine × 80mA = 400mA + ESP32 500mA vârf)

### Zona RF (ESP32)
- Antena modulului IESE în afara conturului PCB sau peste zonă fără cupru
  pe ambele straturi (keep-out conform datasheet Espressif WROOM-32E)
- Plan de masă continuu sub restul modulului
- Fără trasee comutate (relee) sub sau lângă modul

### EMC
- C de 100nF la fiecare CI, cât mai aproape de pinii de alimentare
- Bucla MOV-siguranță-terminal cât mai scurtă
- Ferită SMD opțională pe 5V după PS1 (BLM21PG221 — C18305) — footprint
  prevăzut, populat doar dacă testele EMC o cer

## Date de comandă prototip (serie 10 buc)
| Item | Detaliu |
|------|---------|
| Gerber | Export KiCad 7+ / Altium, inclusiv sloturi în Edge.Cuts |
| Cantitate | 10 buc prototip (5 asamblate SMT + 5 goale) |
| Asamblare | SMT o singură față; releele/terminalele THT manual |
| Stencil | Da, framework-less |
| Cost estimat | PCB ~25 USD + SMT ~90 USD + componente ~140 USD (10 buc, LCSC) |
| Timp | 7–12 zile fabricație + transport |

## Checklist înainte de trimitere
- [ ] DRC trecut cu regulile de creepage 6.4mm definite manual ca net-class
- [ ] Sloturi prezente în Edge.Cuts sub PS1 și relee
- [ ] Poziție găuri montaj M3 ×4 compatibilă cu carcasa aleasă
- [ ] Marcaj serigrafie: model, revizie, săptămâna fabricației (YYWW), pericol 230V
- [ ] Fiducials ×3 pentru asamblare SMT
- [ ] Panelizare 2×2 cu V-score dacă mergeți la serie >50 buc
