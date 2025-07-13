#define BLYNK_TEMPLATE_ID        "TMPL43MLLd5DU"
#define BLYNK_TEMPLATE_NAME      "Power reset"
#define BLYNK_FIRMWARE_VERSION   "2.1.0"
#define BLYNK_PRINT              Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <BlynkSimpleEsp8266.h>
#include <time.h>
#include "../src/settings.cpp"

// — Blynk datastream mapping —
// Rele                -> V0
// Konzole             -> V1
// IP                  -> V2
// WiFi signál         -> V3
// Verze               -> V4
// Deep sleep interval -> V5

Settings settings;

const uint8_t RELAY_PIN = 0;
const uint8_t LED_PIN   = 2;
const long    RESET_TIME = 10000L; // 10 s

BlynkTimer    timer;
WidgetTerminal terminal(V1);
WiFiClient    client;
String        overTheAirURL;

// — timezone offset in seconds (CEST = GMT+2) —  
const long GMT_OFFSET_SEC      = 7200;
const long DAYLIGHT_OFFSET_SEC = 0;

//----------------------------------------------------------------------
// helper: return “[YYYY-MM-DD HH:MM:SS]” in your timezone
String getTimestamp() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char buf[32];
  strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", &timeinfo);
  return String(buf);
}

//----------------------------------------------------------------------
// OTA update via Blynk internal pin
BLYNK_WRITE(InternalPinOTA)
{
  Serial.println("OTA Started");
  overTheAirURL = param.asString();
  Serial.print("overTheAirURL = ");
  Serial.println(overTheAirURL);

  HTTPClient http;
  http.begin(client, overTheAirURL);

  t_httpUpdate_return ret = ESPhttpUpdate.update(client, overTheAirURL);
  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      Serial.println("[update] Update failed.");
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[update] No update available.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("[update] Update ok."); // may not be called since we reboot
      break;
  }
}

//----------------------------------------------------------------------
// sync on connect, send IP/RSSI/version, print startup once
BLYNK_CONNECTED() {
  Blynk.syncAll();

  Blynk.virtualWrite(V2, WiFi.localIP().toString());
  Blynk.virtualWrite(V3, WiFi.RSSI());
  Blynk.virtualWrite(V4, settings.version);
}

//----------------------------------------------------------------------
// called after 10 s to re-close relay
void pulseDone() {
  digitalWrite(RELAY_PIN, HIGH);   // de-energize → NC closes
  digitalWrite(LED_PIN,   HIGH);   // LED off

  terminal.println(getTimestamp() + " - reset done");
  terminal.flush();

  Blynk.virtualWrite(V0, 0);
}

//----------------------------------------------------------------------
// Blynk virtual button V0 pressed → do reset pulse
BLYNK_WRITE(V0) {
  if (param.asInt()) {
    digitalWrite(RELAY_PIN, LOW);  // energize → NC opens
    digitalWrite(LED_PIN,   LOW);  // LED on

    terminal.println(getTimestamp() + " - reset start");
    terminal.flush();

    timer.setTimeout(RESET_TIME, pulseDone);
  }
}

//----------------------------------------------------------------------
// Blynk terminal input handler: "reset" or "restart" → restart ESP
BLYNK_WRITE(V1) {
  String cmd = param.asStr();
  cmd.trim();
  if (cmd.equalsIgnoreCase("reset") || cmd.equalsIgnoreCase("restart")) {
    terminal.println(getTimestamp() + " - restarting...");
    terminal.flush();
    delay(500);
    ESP.restart();
  } else {
    terminal.println("Unknown command: " + cmd);
    terminal.flush();
  }
}

void setup() {
  Serial.begin(115200);

  // init pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN,   OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // default: de-energized (NC closed)
  digitalWrite(LED_PIN,   HIGH);  // LED off

  // connect Blynk (and Wi-Fi)
  Blynk.begin(settings.blynkAuth, settings.wifiSSID, settings.wifiPassword);

  // configure NTP for timestamps
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC,
             "pool.ntp.org", "time.nist.gov");
  // wait for time sync
  time_t now = time(nullptr);
  while (now < 24L * 3600L * 365L) {
    delay(500);
    now = time(nullptr);
  }
  Serial.println("NTP synced: " + getTimestamp());
}

void loop() {
  Blynk.run();
  timer.run();
}
