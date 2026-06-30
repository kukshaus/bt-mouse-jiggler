# Logi-Mouse – Bedienungsanleitung

## Inbetriebnahme
1. ESP32 per USB mit Strom versorgen
2. LED leuchtet **blau** – bereit zur Verbindung
3. Am Computer per Bluetooth nach **Logi-Mouse** suchen und verbinden
4. LED **erlischt** – Maus ist aktiv

## Funktion
Solange verbunden, bewegt die Maus alle 3 Minuten den Cursor 3 Pixel hin und her – der Rechner bleibt wach.

## LED-Status
| LED | Bedeutung |
|-----|-----------|
| Blau leuchtend | Wartet auf Bluetooth-Verbindung |
| Blau blinkend | Verbindung getrennt |
| Aus | Verbunden und aktiv |

## Konfiguration per USB (optional)
USB-Kabel angeschlossen lassen, Terminal öffnen (115200 Baud):

```
1 – Einstellungen anzeigen
2 – Geräte-Name ändern
3 – Intervall ändern (Sekunden)
4 – Pixel ändern
5 – Auf Standard zurücksetzen
6 – Manuell Jiggeln
7 – Demo: Kreis fahren
h – Hilfe
```

Empfohlene Terminal-Programme:
- Windows: PuTTY, Arduino IDE Serieller Monitor
- Mac: `screen /dev/cu.usbmodem* 115200`
- Linux: `screen /dev/ttyACM0 115200`

## Technische Daten
- Mikrocontroller: ESP32-S3
- Bluetooth: BLE HID (Maus)
- Stromversorgung: 5V via USB
