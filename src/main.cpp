#define BLYNK_TEMPLATE_ID "TMPL43MLLd5DU"
#define BLYNK_TEMPLATE_NAME "Power reset"
#define BLYNK_FIRMWARE_VERSION "2.1.0"
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <BlynkSimpleEsp8266.h>
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

// one-shot timer  
BlynkTimer timer;

// Attach Blynk virtual serial terminal
WidgetTerminal terminal(V1);
WiFiClient client;

String overTheAirURL = "";

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
    Serial.println("[update] Update no Update.");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("[update] Update ok."); // may not be called since we reboot the ESP
    break;
  }
}

BLYNK_CONNECTED() {
  Blynk.syncAll();

  String ip = WiFi.localIP().toString();
  Blynk.virtualWrite(V2, ip);

  int rssi = WiFi.RSSI();
  Blynk.virtualWrite(V3, rssi);
  Blynk.virtualWrite(V4, settings.version);
}

void pulseDone() {
  // de-energize relay → NC closes
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  // reset the button in the app
  Blynk.virtualWrite(V0, 0);
}

BLYNK_WRITE(V0) {
  if (param.asInt()) {
    // energize relay → NC opens
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    // schedule it to go back after 10 000 ms
    timer.setTimeout(10000L, pulseDone);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);

  // connect Blynk
  Blynk.begin(settings.blynkAuth, settings.wifiSSID, settings.wifiPassword);
}

void loop() {
  Blynk.run();
  timer.run(); 
}
