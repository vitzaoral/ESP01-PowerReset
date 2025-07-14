# Power Reset – Wi‑Fi “Unplug‑Replug” Switch  
A tiny IoT module that remotely **cuts power for 10 s** and then restores it to whatever is connected (router, Raspberry Pi, AP …).  
After the pulse the device immediately enters deep‑sleep to save power.  
The firmware runs on a **Wemos D1 mini (ESP8266)** and drives an **ESP‑01 Relay Shield** while talking to the **Blynk Cloud** mobile/console app.  
The ESP‑01S module was **not** used—its GPIO 5 does not stay HIGH in deep‑sleep, so the relay would remain latched.

---

## 🗂️ Table of Contents
1. [Key Features](#key-features)  
2. [Hardware](#hardware)  
3. [Wiring](#wiring)  
4. [Project Layout](#project-layout)  
5. [Library Compatibility](#library-compatibility)  
6. [Building & Uploading](#building--uploading)  
7. [`settings.cpp`](#settingscpp)  
8. [Blynk Controls](#blynk-controls)  
9. [Over‑the‑Air Updates](#ota)  
10. [Deep‑Sleep & Power Consumption](#deep-sleep--power-consumption)  
11. [Safety Notes](#safety-notes)  
12. [License](#license)

---

## 1  Key Features<a id="key-features"></a>

| Feature | Details |
| ------- | ------- |
| **Reset pulse** | Relay closes for **10 s** (`RESET_TIME`) and then opens. |
| **Remote trigger** | Virtual button **V0** in the Blynk app. |
| **OTA updates** | Hidden pin `InternalPinOTA`; Blynk sends a `.bin` URL. Timeout 30 s. |
| **Logging** | Time‑stamped messages via NTP to **Blynk Terminal V1** and UART @ 115 200 Bd. |
| **Status LED** | GPIO 2 (on‑board LED) lights while relay is closed / during OTA. |
| **Deep‑sleep** | Sleeps for **15 min** (`SLEEP_SEC`), ~80 µA on Wemos D1 mini. |

---

## 2  Hardware<a id="hardware"></a>

| Part | Notes |
| ---- | ----- |
| **Wemos D1 mini (ESP‑12F)** | 4 MB flash, deep‑sleep capable, exposes GPIO 5 (D1). |
| **ESP‑01 Relay Shield**<br>(Songle SRD‑05VDC‑SL‑C) | Single‑channel 10 A / 250 VAC. |
| 5 V USB power supply | ≥ 500 mA ensures reliable relay switching. |
| Dupont wires | Connections between D1 mini and relay shield. |

> **Why not ESP‑01S?**  
> In deep‑sleep GPIO 5 (exposed only through **CH_PD** on ESP‑01S) does not stay HIGH, leaving the relay latched.

---

## 3  Wiring<a id="wiring"></a>

```
Wemos D1 mini          Relay Shield
 5V  -----------------  VCC
 GND -----------------  GND
 D1 (GPIO5) ----------  IN   (relay control)
 D4 (GPIO2) ----------  LED  (on Wemos itself)
```

Connect the device you want to power‑cycle to the relay’s **COM/NO** terminals  
(typically you break the phase wire of 230 VAC). **Mains voltage is dangerous—take precautions! ⚡**

---

## 4  Project Layout<a id="project-layout"></a>

```
.
├── src
│   ├── main.ino         ← the complete firmware
│   └── settings.cpp     ← secrets (Wi‑Fi SSID, password, Blynk token)
├── platformio.ini / .ino
└── README.md            ← this document
```

---

## 5  Library Compatibility<a id="library-compatibility"></a>

* **Blynk** ≥ 1.4  
* **ESP8266WiFi**, **ESP8266HTTPClient**, **ESP8266httpUpdate**  
* **Arduino core for ESP8266** ≥ 3.1.0

---

## 6  Building & Uploading<a id="building--uploading"></a>

### Arduino IDE

1. Add board URL:  
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. Select **LOLIN (Wemos) D1 R2 & mini** (or **NodeMCU 1.0**).
3. Install libraries via Library Manager (“Blynk”, “ESP8266 HTTP Update”).
4. Open `main.ino`, fill in `settings.cpp`, compile & upload.

### PlatformIO

```ini
[env:d1_mini]
platform      = espressif8266
board         = d1_mini
framework     = arduino
build_flags   = -DBLYNK_PRINT=Serial
upload_speed  = 921600
monitor_speed = 115200
```

```bash
pio run -t upload      # flash firmware
pio device monitor     # view serial log
```

---

## 7  `settings.cpp`<a id="settingscpp"></a>

```cpp
struct Settings {
  const char* wifiSSID       = "••••";
  const char* wifiPassword   = "••••";
  const char* blynkAuth      = "YourBlynkToken";
} settings;
```

Add this file to **`.gitignore`** so you don’t commit secrets.

---

## 8  Blynk Controls<a id="blynk-controls"></a>

| Pin | Widget          | Description |
| --- | --------------- | ----------- |
| **V0** | Button (Switch) | `ON` triggers the 10 s reset pulse; auto‑resets to `OFF`. |
| **V1** | Terminal | Logs: timestamps, relay state, OTA progress. |
| **V2** | Value Display | Device IP address. |
| **V3** | Value Display | Wi‑Fi RSSI (dBm). |
| **V4** | Value Display | Firmware version (`BLYNK_FIRMWARE_VERSION`). |
| **InternalPinOTA** | (hidden) | OTA update – receives the `.bin` URL. |

Requires the **new Blynk Cloud** (v2) account.

---

## 9  Over‑the‑Air Updates<a id="ota"></a>

1. Upload the compiled `.bin` to any reachable HTTPS server.  
2. In Blynk Console → **OTA Update** → paste the file URL.  
3. The ESP8266 downloads it using `ESPhttpUpdate`, verifies the header, flashes, and reboots.  
4. If nothing happens within 30 s (`OTA_TIMEOUT`), OTA aborts and normal operation resumes.

---

## 10  Deep‑Sleep & Power Consumption<a id="deep-sleep--power-consumption"></a>

* After the reset pulse **or** when no reset is requested, the firmware calls  

  ```cpp
  ESP.deepSleep(SLEEP_SEC * 1'000'000ULL);   // 900 s = 15 min
  ```

* Current in deep‑sleep ≈ **80 µA** (bare Wemos D1 mini).  
* Wake‑up jumper: **D0 ↔ RST** (pre‑soldered on D1 mini).

---

## 11  Safety Notes<a id="safety-notes"></a>

* **Mains (230 VAC) wiring must be done by qualified personnel.**  
* The relay is galvanically isolated, but overall enclosure and insulation are your responsibility.  
* Do not exceed 10 A / 250 VAC contact rating.  
* Not intended for life‑support or safety‑critical systems.

---

## 12  License<a id="license"></a>

This firmware is released under the **MIT License**.  
Third‑party library licenses (e.g. **Blynk**) apply independently.

---

**Happy hacking!**  
Questions or improvements? Open an issue 🛠️
