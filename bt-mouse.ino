#include <BleMouse.h>
#include <Preferences.h>
#include "esp_random.h"

// ----------------------------------------------------------------
// Logitech-Modellliste (echte, am Markt erhältliche Mäuse)
// Der gewählte Name erscheint beim Bluetooth-Pairing -> "unauffällig"
// ----------------------------------------------------------------
const char *LOGI_MODELS[] = {
  "MX Master 3S",
  "MX Master 2S",
  "MX Anywhere 3",
  "MX Anywhere 2S",
  "M720 Triathlon",
  "M585 Multi-Device",
  "M510",
  "M325",
  "M185",
  "Pebble M350",
  "Signature M650",
  "G Pro X Superlight"
};
const int LOGI_COUNT = sizeof(LOGI_MODELS) / sizeof(LOGI_MODELS[0]);

// Standardwerte
#define DEFAULT_MODEL_IDX 0          // "MX Master 3S"
#define DEFAULT_INT_MIN_MS 60000UL   // 1 Minute
#define DEFAULT_INT_MAX_MS 120000UL  // 2 Minuten
#define DEFAULT_DISTANCE 3

// LED
#define USE_RGB_BUILTIN true
#define RGB_LED_PIN 48
#define USE_STANDARD_LED true
#define STANDARD_LED_PIN 2
#define BLINK_MS 500

Preferences prefs;

// Laufzeit-Variablen (aus Preferences geladen)
String deviceName = LOGI_MODELS[DEFAULT_MODEL_IDX];
unsigned long intMinMs = DEFAULT_INT_MIN_MS;
unsigned long intMaxMs = DEFAULT_INT_MAX_MS;
int distance = DEFAULT_DISTANCE;

// Zeitplan
bool scheduleEnabled = false;
int startMin = 9 * 60;   // 09:00 (Minuten seit Mitternacht)
int endMin = 17 * 60;    // 17:00

// Software-Uhr (kein RTC/WLAN vorhanden -> Zeit wird per Serial gesetzt)
bool clockSet = false;
unsigned long clockBaseMs = 0;  // millis() beim Setzen
long clockBaseSec = 0;          // Sekunden seit Mitternacht beim Setzen

unsigned long lastJiggle = 0;
unsigned long nextInterval = DEFAULT_INT_MAX_MS;
bool lastConn = false;
unsigned long lastBlink = 0;
bool blinkState = false;

BleMouse *mouse = nullptr;

// ----------------------------------------------------------------
// LED
// ----------------------------------------------------------------
void updLED(bool on) {
  if (USE_RGB_BUILTIN) neopixelWrite(RGB_LED_PIN, 0, 0, on ? 0 : 30);
  if (USE_STANDARD_LED) digitalWrite(STANDARD_LED_PIN, on ? LOW : HIGH);
}

// ----------------------------------------------------------------
// Software-Uhr
// ----------------------------------------------------------------
long curSecOfDay() {
  if (!clockSet) return -1;
  unsigned long elapsed = (millis() - clockBaseMs) / 1000UL;
  return (clockBaseSec + (long)elapsed) % 86400L;
}

int curMinOfDay() {
  long s = curSecOfDay();
  return s < 0 ? -1 : (int)(s / 60);
}

// Liegt "jetzt" im aktiven Zeitfenster?
bool inSchedule() {
  if (!scheduleEnabled) return true;
  if (!clockSet) return true;  // ohne gesetzte Zeit kein Plan -> immer aktiv
  int now = curMinOfDay();
  if (startMin == endMin) return true;  // 24 h
  if (startMin < endMin) return now >= startMin && now < endMin;
  return now >= startMin || now < endMin;  // über Mitternacht
}

// ----------------------------------------------------------------
// Zufalls-Intervall
// ----------------------------------------------------------------
unsigned long randInterval() {
  if (intMaxMs <= intMinMs) return intMinMs;
  return intMinMs + (unsigned long)random(0, (long)(intMaxMs - intMinMs) + 1);
}

// ----------------------------------------------------------------
// Preferences
// ----------------------------------------------------------------
void loadCfg() {
  prefs.begin("lm", false);
  deviceName = prefs.getString("name", LOGI_MODELS[DEFAULT_MODEL_IDX]);
  intMinMs = prefs.getULong("imin", DEFAULT_INT_MIN_MS);
  intMaxMs = prefs.getULong("imax", DEFAULT_INT_MAX_MS);
  distance = prefs.getInt("dist", DEFAULT_DISTANCE);
  scheduleEnabled = prefs.getBool("sch", false);
  startMin = prefs.getInt("smin", 9 * 60);
  endMin = prefs.getInt("emin", 17 * 60);
  prefs.end();
  if (intMinMs < 1000) intMinMs = DEFAULT_INT_MIN_MS;
  if (intMaxMs < intMinMs) intMaxMs = intMinMs;
  if (distance < 1 || distance > 100) distance = DEFAULT_DISTANCE;
  if (startMin < 0 || startMin > 1439) startMin = 9 * 60;
  if (endMin < 0 || endMin > 1439) endMin = 17 * 60;
}

void saveCfg() {
  prefs.begin("lm", false);
  prefs.putString("name", deviceName);
  prefs.putULong("imin", intMinMs);
  prefs.putULong("imax", intMaxMs);
  prefs.putInt("dist", distance);
  prefs.putBool("sch", scheduleEnabled);
  prefs.putInt("smin", startMin);
  prefs.putInt("emin", endMin);
  prefs.end();
}

void resetCfg() {
  deviceName = LOGI_MODELS[DEFAULT_MODEL_IDX];
  intMinMs = DEFAULT_INT_MIN_MS;
  intMaxMs = DEFAULT_INT_MAX_MS;
  distance = DEFAULT_DISTANCE;
  scheduleEnabled = false;
  startMin = 9 * 60;
  endMin = 17 * 60;
  saveCfg();
}

// ----------------------------------------------------------------
// BLE Mouse Start/Neustart
// ----------------------------------------------------------------
void startMouse() {
  if (mouse) {
    mouse->end();
    delete mouse;
  }
  // Hersteller "Logitech" + echter Modellname -> erscheint als echte Maus
  mouse = new BleMouse(deviceName.c_str(), "Logitech", 100);
  mouse->begin();
  Serial.printf("[BLE] Gestartet als '%s' (Logitech)\n", deviceName.c_str());
}

// ----------------------------------------------------------------
// Jiggler
// ----------------------------------------------------------------
void jiggle() {
  if (!mouse || !mouse->isConnected()) return;
  mouse->move(distance, 0);
  delay(150);
  mouse->move(-distance, 0);
}

// ----------------------------------------------------------------
// Demo: Kreis fahren
// ----------------------------------------------------------------
void demoKreis() {
  if (!mouse || !mouse->isConnected()) {
    Serial.println(F("Nicht verbunden!"));
    return;
  }
  Serial.println(F("Demo: Kreis (100px, 3s)"));
  int r = 50;
  int n = 100;
  int dt = 3000 / n;
  float px[n + 1], py[n + 1];
  for (int i = 0; i <= n; i++) {
    float a = 2.0 * PI * i / n;
    px[i] = r * cos(a);
    py[i] = r * sin(a);
  }
  for (int i = 1; i <= n; i++) {
    signed char dx = (signed char)round(px[i] - px[i - 1]);
    signed char dy = (signed char)round(py[i] - py[i - 1]);
    mouse->move(dx, dy);
    delay(dt);
  }
  Serial.println(F("Demo fertig!"));
}

// ----------------------------------------------------------------
// Hilfsfunktion: HH:MM einlesen -> Minuten seit Mitternacht (-1 = ungültig)
// ----------------------------------------------------------------
int parseHHMM(const String &s) {
  int colon = s.indexOf(':');
  if (colon < 1) return -1;
  int h = s.substring(0, colon).toInt();
  int m = s.substring(colon + 1).toInt();
  if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
  return h * 60 + m;
}

String fmtHHMM(int minOfDay) {
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", minOfDay / 60, minOfDay % 60);
  return String(buf);
}

String readLine() {
  while (!Serial.available()) { delay(10); }
  String input = Serial.readStringUntil('\n');
  input.trim();
  return input;
}

// ----------------------------------------------------------------
// Serielles Menü
// ----------------------------------------------------------------
void menuHilfe() {
  Serial.println(F("\n========== Logi-Mouse Menü =========="));
  Serial.println(F(" 1  – Einstellungen anzeigen"));
  Serial.println(F(" 2  – Geräte-Modell wählen (Logitech-Liste)"));
  Serial.println(F(" 3  – Intervall-Bereich ändern (zufällig)"));
  Serial.println(F(" 4  – Strecke ändern (Pixel)"));
  Serial.println(F(" 5  – Aktuelle Uhrzeit setzen (HH:MM)"));
  Serial.println(F(" 6  – Zeitplan konfigurieren"));
  Serial.println(F(" 7  – Auf Standard zurücksetzen"));
  Serial.println(F(" 8  – Manuell Jiggeln"));
  Serial.println(F(" 9  – Demo: Kreis fahren (100px, 3s)"));
  Serial.println(F(" h  – Dieses Menü"));
  Serial.println(F("======================================"));
  Serial.print(F("> "));
}

void menuZeigen() {
  Serial.println(F("--------- Aktuelle Einstellungen ---------"));
  Serial.printf("Modell:    %s (Logitech)\n", deviceName.c_str());
  Serial.printf("Intervall: %lu-%lu s (zufällig)\n", intMinMs / 1000, intMaxMs / 1000);
  Serial.printf("Pixel:     %d\n", distance);
  if (clockSet) {
    long s = curSecOfDay();
    Serial.printf("Uhrzeit:   %02ld:%02ld\n", s / 3600, (s % 3600) / 60);
  } else {
    Serial.println("Uhrzeit:   nicht gesetzt");
  }
  if (scheduleEnabled) {
    Serial.printf("Zeitplan:  AN  %s -> %s  (jetzt: %s)\n",
                  fmtHHMM(startMin).c_str(), fmtHHMM(endMin).c_str(),
                  inSchedule() ? "aktiv" : "pausiert");
  } else {
    Serial.println("Zeitplan:  AUS (immer aktiv)");
  }
  Serial.printf("BLE:       %s\n", mouse && mouse->isConnected() ? "Verbunden" : "Nicht verbunden");
}

void menuModell() {
  Serial.println(F("Verfügbare Logitech-Modelle:"));
  for (int i = 0; i < LOGI_COUNT; i++) {
    Serial.printf("  %2d – %s%s\n", i + 1, LOGI_MODELS[i],
                  deviceName == LOGI_MODELS[i] ? "  (aktuell)" : "");
  }
  Serial.print(F("Nummer wählen (Enter = abbrechen): "));
  String input = readLine();
  if (input.length() == 0) {
    Serial.println("Abgebrochen");
    return;
  }
  int idx = input.toInt();
  if (idx >= 1 && idx <= LOGI_COUNT) {
    deviceName = LOGI_MODELS[idx - 1];
    saveCfg();
    // Sauberer Neustart: BLE-Stack komplett neu initialisieren
    // (Library gibt GATT-Handles bei mouse->end() nicht vollständig frei)
    Serial.printf("Modell geändert auf '%s' – Neustart...\n", deviceName.c_str());
    Serial.flush();
    delay(200);
    ESP.restart();
  } else {
    Serial.printf("Ungültig (1-%d)\n", LOGI_COUNT);
  }
}

void menuIntervall() {
  Serial.print(F("Min-Intervall in Sekunden (1-3600, Enter = abbrechen): "));
  String a = readLine();
  if (a.length() == 0) { Serial.println("Abgebrochen"); return; }
  unsigned long mn = a.toInt();
  Serial.print(F("Max-Intervall in Sekunden (>= Min, <= 3600): "));
  String b = readLine();
  if (b.length() == 0) { Serial.println("Abgebrochen"); return; }
  unsigned long mx = b.toInt();
  if (mn < 1 || mn > 3600 || mx < mn || mx > 3600) {
    Serial.println("Ungültig (1-3600, Max >= Min)");
    return;
  }
  intMinMs = mn * 1000UL;
  intMaxMs = mx * 1000UL;
  saveCfg();
  lastJiggle = millis();
  nextInterval = randInterval();
  Serial.printf("Intervall geändert auf %lu-%lu s (zufällig)\n", mn, mx);
}

void menuPixel() {
  Serial.print(F("Neue Pixel (1-100, Enter = abbrechen): "));
  String input = readLine();
  if (input.length() > 0) {
    int val = input.toInt();
    if (val >= 1 && val <= 100) {
      distance = val;
      saveCfg();
      Serial.printf("Pixel geändert auf %d\n", val);
    } else {
      Serial.println("Ungültig (1-100)");
    }
  } else {
    Serial.println("Abgebrochen");
  }
}

void menuUhrzeit() {
  Serial.print(F("Aktuelle Uhrzeit als HH:MM (Enter = abbrechen): "));
  String input = readLine();
  if (input.length() == 0) { Serial.println("Abgebrochen"); return; }
  int m = parseHHMM(input);
  if (m < 0) {
    Serial.println("Ungültig (Format HH:MM)");
    return;
  }
  clockBaseSec = (long)m * 60L;
  clockBaseMs = millis();
  clockSet = true;
  Serial.printf("Uhrzeit gesetzt auf %s\n", fmtHHMM(m).c_str());
}

void menuZeitplan() {
  Serial.printf("Zeitplan ist aktuell: %s\n", scheduleEnabled ? "AN" : "AUS");
  Serial.print(F("Zeitplan aktivieren? (j = an, n = aus, Enter = abbrechen): "));
  String onoff = readLine();
  if (onoff.length() == 0) { Serial.println("Abgebrochen"); return; }

  if (onoff.equalsIgnoreCase("n")) {
    scheduleEnabled = false;
    saveCfg();
    Serial.println("Zeitplan AUS – Jiggler immer aktiv");
    return;
  }
  if (!onoff.equalsIgnoreCase("j")) {
    Serial.println("Abgebrochen");
    return;
  }

  Serial.print(F("Start-Zeit HH:MM: "));
  int s = parseHHMM(readLine());
  if (s < 0) { Serial.println("Ungültig (Format HH:MM)"); return; }
  Serial.print(F("End-Zeit HH:MM: "));
  int e = parseHHMM(readLine());
  if (e < 0) { Serial.println("Ungültig (Format HH:MM)"); return; }

  startMin = s;
  endMin = e;
  scheduleEnabled = true;
  saveCfg();
  Serial.printf("Zeitplan AN: %s -> %s\n", fmtHHMM(startMin).c_str(), fmtHHMM(endMin).c_str());
  if (!clockSet) {
    Serial.println("WARNUNG: Uhrzeit nicht gesetzt (Menü 5) – Plan wird ignoriert!");
  }
}

void menuReset() {
  Serial.print(F("Wirklich auf Standard zurücksetzen? (j/n): "));
  String input = readLine();
  if (input.equalsIgnoreCase("j")) {
    resetCfg();
    Serial.println("Auf Standard zurückgesetzt – Neustart...");
    Serial.flush();
    delay(200);
    ESP.restart();
  } else {
    Serial.println("Abgebrochen");
  }
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) { menuHilfe(); return; }

  switch (cmd[0]) {
    case '1': menuZeigen(); break;
    case '2': menuModell(); break;
    case '3': menuIntervall(); break;
    case '4': menuPixel(); break;
    case '5': menuUhrzeit(); break;
    case '6': menuZeitplan(); break;
    case '7': menuReset(); break;
    case '8':
      jiggle();
      Serial.println("Manuell gejiggelt!");
      break;
    case '9': demoKreis(); break;
    case 'h': case 'H': menuHilfe(); break;
    default:  Serial.println("Unbekannt. 'h' für Menü"); break;
  }
  Serial.print(F("\n> "));
}

// ----------------------------------------------------------------
// Setup
// ----------------------------------------------------------------
void setup() {
  Serial.begin(115200); delay(500);
  Serial.println(F("\n=== Logi-Mouse ==="));

  randomSeed(esp_random());

  if (USE_STANDARD_LED) pinMode(STANDARD_LED_PIN, OUTPUT);
  updLED(false);

  loadCfg();
  startMouse();

  menuHilfe();
  lastJiggle = millis();
  nextInterval = randInterval();
}

// ----------------------------------------------------------------
// Loop
// ----------------------------------------------------------------
void loop() {
  bool conn = mouse && mouse->isConnected();
  if (conn != lastConn) {
    lastConn = conn;
    updLED(conn);
    Serial.println(conn ? "[BLE] Verbunden!" : "[BLE] Getrennt!");
    if (conn) {
      lastJiggle = millis();
      nextInterval = randInterval();
    }
  }
  if (!conn) {
    if (millis() - lastBlink >= BLINK_MS) {
      lastBlink = millis(); blinkState = !blinkState;
      if (USE_RGB_BUILTIN) neopixelWrite(RGB_LED_PIN, 0, 0, blinkState ? 30 : 0);
      if (USE_STANDARD_LED) digitalWrite(STANDARD_LED_PIN, blinkState ? HIGH : LOW);
    }
  }
  if (conn && inSchedule() && millis() - lastJiggle >= nextInterval) {
    jiggle();
    lastJiggle = millis();
    nextInterval = randInterval();  // nächstes Intervall neu auswürfeln
  }
  handleSerial();
  delay(10);
}
