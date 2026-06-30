#include <BleMouse.h>
#include <Preferences.h>

// Standardwerte
#define DEFAULT_NAME "Logi-Mouse"
#define DEFAULT_INTERVAL_MS 180000
#define DEFAULT_DISTANCE 3

// LED
#define USE_RGB_BUILTIN true
#define RGB_LED_PIN 48
#define USE_STANDARD_LED true
#define STANDARD_LED_PIN 2
#define BLINK_MS 500

Preferences prefs;

// Laufzeit-Variablen (aus Preferences geladen)
String deviceName = DEFAULT_NAME;
unsigned long intervalMs = DEFAULT_INTERVAL_MS;
int distance = DEFAULT_DISTANCE;

unsigned long lastJiggle = 0;
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
// Preferences
// ----------------------------------------------------------------
void loadCfg() {
  prefs.begin("lm", false);
  deviceName  = prefs.getString("name", DEFAULT_NAME);
  intervalMs  = prefs.getULong("int", DEFAULT_INTERVAL_MS);
  distance    = prefs.getInt("dist", DEFAULT_DISTANCE);
  prefs.end();
  if (intervalMs < 1000) intervalMs = DEFAULT_INTERVAL_MS;
  if (distance < 1 || distance > 100) distance = DEFAULT_DISTANCE;
}

void saveCfg() {
  prefs.begin("lm", false);
  prefs.putString("name", deviceName);
  prefs.putULong("int", intervalMs);
  prefs.putInt("dist", distance);
  prefs.end();
}

void resetCfg() {
  deviceName = DEFAULT_NAME;
  intervalMs = DEFAULT_INTERVAL_MS;
  distance = DEFAULT_DISTANCE;
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
  mouse = new BleMouse(deviceName.c_str(), "Logitech", 100);
  mouse->begin();
  Serial.printf("[BLE] Gestartet als '%s'\n", deviceName.c_str());
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
// Serielles Menü
// ----------------------------------------------------------------
void menuHilfe() {
  Serial.println(F("\n========== Logi-Mouse Menü =========="));
  Serial.println(F(" 1  – Einstellungen anzeigen"));
  Serial.println(F(" 2  – Geräte-Name ändern"));
  Serial.println(F(" 3  – Intervall ändern (Sekunden)"));
  Serial.println(F(" 4  – Strecke ändern (Pixel)"));
  Serial.println(F(" 5  – Auf Standard zurücksetzen"));
  Serial.println(F(" 6  – Manuell Jiggeln"));
  Serial.println(F(" 7  – Demo: Kreis fahren (100px, 3s)"));
  Serial.println(F(" h  – Dieses Menü"));
  Serial.println(F("======================================"));
  Serial.print(F("> "));
}

void menuZeigen() {
  Serial.println(F("--------- Aktuelle Einstellungen ---------"));
  Serial.printf("Name:      %s\n", deviceName.c_str());
  Serial.printf("Intervall: %lu s (%lu ms)\n", intervalMs / 1000, intervalMs);
  Serial.printf("Pixel:     %d\n", distance);
  Serial.printf("BLE:       %s\n", mouse && mouse->isConnected() ? "Verbunden" : "Nicht verbunden");
}

void menuName() {
  Serial.print(F("Neuer Geräte-Name (Enter = abbrechen): "));
  while (!Serial.available()) { delay(10); }
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() > 0 && input.length() <= 30) {
    deviceName = input;
    saveCfg();
    startMouse();
    Serial.printf("Name geändert auf '%s'\n", deviceName.c_str());
  } else {
    Serial.println("Abgebrochen (max 30 Zeichen)");
  }
}

void menuIntervall() {
  Serial.printf("Neues Intervall in Sekunden (%lu-%lu, Enter = abbrechen): ", 1UL, 3600UL);
  while (!Serial.available()) { delay(10); }
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() > 0) {
    unsigned long val = input.toInt();
    if (val >= 1 && val <= 3600) {
      intervalMs = val * 1000;
      saveCfg();
      lastJiggle = millis();
      Serial.printf("Intervall geändert auf %lu s\n", val);
    } else {
      Serial.println("Ungültig (1-3600)");
    }
  } else {
    Serial.println("Abgebrochen");
  }
}

void menuPixel() {
  Serial.print(F("Neue Pixel (1-100, Enter = abbrechen): "));
  while (!Serial.available()) { delay(10); }
  String input = Serial.readStringUntil('\n');
  input.trim();
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

void menuReset() {
  Serial.print(F("Wirklich auf Standard zurücksetzen? (j/n): "));
  while (!Serial.available()) { delay(10); }
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.equalsIgnoreCase("j")) {
    resetCfg();
    startMouse();
    Serial.println("Auf Standard zurückgesetzt!");
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
    case '2': menuName(); break;
    case '3': menuIntervall(); break;
    case '4': menuPixel(); break;
    case '5': menuReset(); break;
    case '6':
      jiggle();
      Serial.println("Manuell gejiggelt!");
      break;
    case '7': demoKreis(); break;
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

  if (USE_STANDARD_LED) pinMode(STANDARD_LED_PIN, OUTPUT);
  updLED(false);

  loadCfg();
  startMouse();

  menuHilfe();
  lastJiggle = millis();
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
    if (conn) lastJiggle = millis();
  }
  if (!conn) {
    if (millis() - lastBlink >= BLINK_MS) {
      lastBlink = millis(); blinkState = !blinkState;
      if (USE_RGB_BUILTIN) neopixelWrite(RGB_LED_PIN, 0, 0, blinkState ? 30 : 0);
      if (USE_STANDARD_LED) digitalWrite(STANDARD_LED_PIN, blinkState ? HIGH : LOW);
    }
  }
  if (conn && millis() - lastJiggle >= intervalMs) {
    jiggle();
    lastJiggle = millis();
  }
  handleSerial();
  delay(10);
}