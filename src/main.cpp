#define BLYNK_TEMPLATE_ID        "TMPL43MLLd5DU"
#define BLYNK_TEMPLATE_NAME      "Power reset"
#define BLYNK_FIRMWARE_VERSION   "2.1.0"
#define BLYNK_PRINT              Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <BlynkSimpleEsp8266.h>
#include <time.h>
#include "../src/settings.cpp"

Settings settings;

// I/O pins and timing
const uint8_t  RELAY_PIN       = 5; // GPIO5 == D1
const uint8_t  LED_PIN         = 2;
const unsigned long RESET_TIME = 10000UL;  // 10 s pulse
const unsigned long SLEEP_SEC  = 15UL;     // 15 s deep-sleep

volatile bool resetPending = false;

// Blynk terminal on V1
WidgetTerminal terminal(V1);

// --- Timestamp helper ---
const long GMT_OFFSET_SEC      = 7200;  // CEST
const long DAYLIGHT_OFFSET_SEC = 0;

String getTimestamp() {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  char buf[32];
  strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", &t);
  return String(buf);
}

// --- go to hardware deep-sleep ---
void goToSleep() {
  String ts = getTimestamp();
  terminal.println(ts + " - sleeping");
  terminal.flush();
  Blynk.disconnect();
  WiFi.disconnect(true);
  delay(100);
  ESP.deepSleep(SLEEP_SEC * 1000000ULL);
}

// --- perform reset pulse then sleep ---
void doResetPulse() {
  String ts1 = getTimestamp();
  terminal.println(ts1 + " - reset start");
  terminal.flush();

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN,   LOW);

  unsigned long t0 = millis();
  while (millis() - t0 < RESET_TIME) {
    Blynk.run();  // service Blynk to avoid WDT
    delay(10);
  }

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN,   HIGH);
  Blynk.virtualWrite(V0, 0);

  String ts2 = getTimestamp();
  terminal.println(ts2 + " - reset done");
  terminal.flush();

  goToSleep();
}

// --- catch the virtual button press ---
BLYNK_WRITE(V0) {
  if (param.asInt()) {
    resetPending = true;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN,   OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN,   HIGH);

  // configure NTP for timestamps
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC,
             "pool.ntp.org", "time.nist.gov");
  // wait up to 3 s for time to sync
  time_t now = time(nullptr);
  unsigned long start = millis();
  while (now < 24L*3600L*365L && millis() - start < 3000) {
    delay(200);
    now = time(nullptr);
  }

  // connect to Blynk (with built-in Wi-Fi)
  Blynk.begin(settings.blynkAuth, settings.wifiSSID, settings.wifiPassword);

  // wait up to 5 s for Blynk to connect
  unsigned long t0 = millis();
  while (!Blynk.connected() && millis() - t0 < 5000) {
    Blynk.run();
  }

  // sync only V0’s state (will set resetPending if ON)
  Blynk.syncVirtual(V0);

  // give the sync a moment to invoke the handler
  unsigned long s0 = millis();
  while (millis() - s0 < 2000) {
    Blynk.run();
  }

  // decide: reset or sleep
  if (resetPending) {
    doResetPulse();
  } else {
    goToSleep();
  }
}

void loop() {
  // never reaches here—everything is handled in setup()
}
