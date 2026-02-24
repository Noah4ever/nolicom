## Hardware - Variante B (saubere Integration mit Power-Path)

Diese Version nutzt einen **Power-Path LiPo-Ladecontroller** (z. B. MCP73871)  
und trennt Stromversorgung, Laden und Datenübertragung sauber voneinander.  
So kann das Gerät **über USB-C betrieben und geladen werden** - gleichzeitig -  
ohne dass der Akku oder das ESP32-Board durch Spannungsschwankungen gestört wird.

### Blockdiagramm

USB-C Port  
 ├── **VBUS (5 V)** → MCP73871 (LiPo-Ladecontroller mit Power-Path)  
 │ ├── **SYS** → 3.3 V-Regler → ESP32 WROOM-32  
 │ └── **BAT** → LiPo-Akku 1S (z. B. 1000 mAh)  
 ├── **D+ / D-** → USB-UART-Chip (CP2102 / CH340) → ESP32 UART0  
 └── **GND** → gemeinsame Masse aller Komponenten

---

### Komponentenliste (BOM)

| Komponente                      | Beschreibung / Empfehlung                                                            |
| ------------------------------- | ------------------------------------------------------------------------------------ |
| **ESP32-WROOM-32 Modul**        | Nackt oder als DevKit ohne eigenen Lader                                             |
| **USB-UART-Bridge**             | CP2102, CH340 oder FTDI; mit Auto-Reset/Boot-Schaltung                               |
| **MCP73871 Lade-IC**            | 1S LiPo-Lader mit Power-Path (USB-Betrieb + Laden gleichzeitig)                      |
| **3.3 V-Regler**                | Low-Iq LDO (MCP1700) oder effizienter Buck (TPS62160)                                |
| **LiPo-Akku**                   | 1S, 500-2000 mAh, JST-PH 2.0 Stecker                                                 |
| **USB-C Buchse**                | Mit 5.1 kΩ CC-Pull-Downs für 5 V Negotiation                                         |
| **Schiebeschalter (DPST)**      | C64-Style Kippschalter, trennt SYS→3.3 V und optional VBUS                           |
| **Button (Hauptknopf)**         | Beiger rechteckiger Taster, GPIO→GND, für Ping-Funktion                              |
| **Poti 10 kΩ linear**           | Für LED-Helligkeit (an ADC-Pin)                                                      |
| **Piezo-Buzzer (passiv)**       | Für Retro-Sounds (über `tone()` steuerbar)                                           |
| **LEDs**                        | Rot, Gelb, Grün, diffus, 5 mm, je 220 Ω Vorwiderstand                                |
| **2×7-Segment-Display**         | TM1637-Modul, 0.36" oder 0.56", Anzeige von Status/Werten                            |
| **Kondensatoren**               | 100 µF Elko + 100 nF Keramik nahe 3V3/GND                                            |
| **Batterie-Mess-IC (optional)** | MAX17048 Fuel-Gauge für genaue Akkuanzeige                                           |
| **Kleinteile**                  | JST-PH Buchsen, Kabel, Abstandshalter, Schrauben, TVS-Diode, Polyfuse, Isolierhülsen |

---

### Verdrahtung (wichtigste Punkte)

1. **USB-C Port**
   - **VBUS (5 V)** → MCP73871 `VDD`
   - **D+ / D-** → USB-UART-Chip → ESP32 `U0TXD`/`U0RXD`
   - **GND** überall gemeinsam

2. **MCP73871**
   - `VBAT` → LiPo-Akku (JST-PH)
   - `SYS` → Eingang 3.3 V-Regler → ESP32 3V3
   - `PROG` → Ladestrom-Widerstand (z. B. 2 kΩ für ~500 mA)
   - `STAT1`/`STAT2` → Status-LEDs (Laden/Fertig)

3. **Schalter (DPST)**
   - Schaltet `SYS`→3.3 V-Regler hart ab
   - Optional zweite Polung: VBUS-Trennung für echtes „aus“

4. **Bedienelemente**
   - **Poti**: 3V3 - Poti - GND, Schleifer → ADC (GPIO34-39)
   - **Buzzer**: GPIO → 100 Ω → Piezo → GND
   - **LEDs**: GPIO → 220 Ω → LED → GND
   - **Button**: GPIO33 → Taster → GND (`INPUT_PULLUP`)

---

### Vorteile dieser Lösung

- **Gleichzeitiges Laden & Betrieb** ohne Unterbrechung (Power-Path)
- **Keine Spannungseinbrüche** bei Lastwechsel
- **Ein USB-C-Port** für Laden _und_ Flashen
- **Saubere Abschaltung** über DPST-Schalter
- Modularer Aufbau → leicht wartbar und erweiterbar

---

### Hinweise

- MCP73871 hat thermische Limitierungen: Ladestrom an Akku- und Gehäusegröße anpassen
- Bei Einsatz von Buck-Regler → höhere Effizienz bei Wi-Fi-Last
- Immer **gemeinsame Masse** führen
- LED-Vorwiderstände abhängig von LED-Farbe & Strom anpassen
