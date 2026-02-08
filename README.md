# Heltec Master LoRa – BLE NUS Monitor

<p align="center">
  <img src="heltec_modul.png" alt="Heltec WiFi LoRa 32 V3" width="400">
</p>

Ein modulares ESP32-Embedded-Projekt auf Basis des **Heltec WiFi LoRa 32 V3**, das per **Bluetooth Low Energy (BLE)** mit einer Flutter-Android-App kommuniziert. Die Kommunikation erfolgt ueber den **Nordic UART Service (NUS)** mit JSON-basiertem Protokoll.

---

## Inhaltsverzeichnis

1. [Projektuebersicht](#projektuebersicht)
2. [Hardware](#hardware)
3. [Systemarchitektur](#systemarchitektur)
4. [Modulbeschreibung](#modulbeschreibung)
5. [BLE-Architektur](#ble-architektur)
6. [OLED-Display](#oled-display)
7. [Konfiguration](#konfiguration)
8. [Entwicklungsumgebung einrichten](#entwicklungsumgebung-einrichten)
9. [Build, Upload und Monitor](#build-upload-und-monitor)
10. [Projektstruktur](#projektstruktur)
11. [Ablauf (Setup / Loop)](#ablauf-setup--loop)
12. [Geplante Erweiterungen](#geplante-erweiterungen)

---

## Projektuebersicht

| Eigenschaft       | Wert                                      |
|-------------------|--------------------------------------------|
| **Projektname**   | Heltec Master                              |
| **Version**       | 0.1.0                                      |
| **Hardware**      | Heltec WiFi LoRa 32 V3 (ESP32-S3)         |
| **Framework**     | Arduino (PlatformIO)                       |
| **BLE Stack**     | NimBLE-Arduino (h2zero)                    |
| **Display**       | SSD1306 OLED 128x64 (I2C, U8g2)           |
| **Companion App** | Flutter Android (flutter_blue_plus)        |

Das Geraet dient als:
- BLE Peripheral mit Nordic UART Service
- Echtzeit-Uhr (NTP-synchronisiert)
- OLED-Statusmonitor
- Grundlage fuer kuenftige Steuer- und Monitoring-Funktionen

---

## Hardware

### Heltec WiFi LoRa 32 V3

| Komponente   | Details                        |
|-------------|--------------------------------|
| MCU         | ESP32-S3                       |
| OLED        | SSD1306, 128x64, I2C           |
| BLE         | Integriert (NimBLE Stack)      |
| WiFi        | 802.11 b/g/n (nur fuer NTP)   |
| LoRa        | SX1262 (vorbereitet, noch nicht aktiv) |

### Pin-Belegung

| Funktion     | GPIO |
|-------------|------|
| OLED SDA    | 17   |
| OLED SCL    | 18   |
| OLED Reset  | 21   |
| Vext (OLED Power) | 36 |

> **Vext** steuert die Stromversorgung des OLED-Displays. Active LOW: `LOW` = Ein, `HIGH` = Aus.

---

## Systemarchitektur

```
+------------------+        BLE (NUS)        +------------------+
|  Heltec ESP32    | <---------------------> |  Flutter App     |
|  (Peripheral)    |    JSON ueber TX/RX     |  (Central)       |
+------------------+                         +------------------+
        |
        |--- Board       (Hardware-Init)
        |--- Logger      (Circular Log Buffer)
        |--- Ui          (OLED Rendering)
        |--- TimeSvc     (WiFi + NTP Sync)
        |--- BleSvc      (BLE NUS Server)
```

Die Module sind voneinander entkoppelt und kommunizieren ueber klar definierte Schnittstellen. Die zentrale Konfiguration erfolgt in `include/config.h` ueber `#define`-Makros.

---

## Modulbeschreibung

### Board (`lib/Board/`)

Hardware-Abstraktionsschicht fuer den Heltec-Board-spezifischen Init-Prozess.

| Funktion              | Beschreibung                               |
|-----------------------|--------------------------------------------|
| `Board::vextOn()`     | Aktiviert Vext (OLED-Stromversorgung)      |
| `Board::oledResetPulse()` | Reset-Puls fuer das OLED (LOW-HIGH)    |
| `Board::i2cBeginForOled()` | Initialisiert I2C-Bus (SDA/SCL/Clock) |

### Logger (`lib/Logger/`)

Ringpuffer-basiertes Logging-System. Schreibt gleichzeitig auf Serial und in einen RAM-Buffer.

| Methode                    | Beschreibung                                    |
|----------------------------|-------------------------------------------------|
| `Logger(size_t lines)`     | Konstruktor, legt Puffergroesse fest (default: 3)|
| `begin(uint32_t baud)`     | Initialisiert Serial mit angegebener Baudrate    |
| `log(const String& s)`     | Loggt Nachricht auf Serial + Ringpuffer          |
| `count()`                  | Anzahl aktuell gespeicherter Zeilen              |
| `getOldestFirst(size_t i)` | Gibt i-te Zeile in FIFO-Reihenfolge zurueck      |

Der Ringpuffer speichert die letzten **3 Zeilen** (konfigurierbar via `LOGGER_LINES`), was fuer die OLED-Anzeige optimiert ist.

### TimeSvc (`lib/TimeSvc/`)

Einmalige Zeitsynchronisation: WiFi verbinden, NTP abrufen, WiFi trennen.

| Funktion                                | Beschreibung                              |
|-----------------------------------------|-------------------------------------------|
| `TimeSvc::valid()`                      | Prueft ob Systemzeit > Epoch-Schwelle     |
| `TimeSvc::formatHHMMSS(buf, len)`       | Formatiert aktuelle Zeit als `HH:MM:SS`   |
| `TimeSvc::syncOnce(ssid, pass, tz, fn)` | Einmalige WiFi+NTP Synchronisation        |

**Ablauf von `syncOnce`:**
1. WiFi im STA-Modus verbinden (Timeout: 12 Sekunden)
2. Zeitzone setzen (`TZ_INFO`)
3. NTP-Server abfragen (3 Server konfiguriert)
4. Warten auf gueltige Zeit (Timeout: 12 Sekunden)
5. WiFi trennen und deaktivieren

### Ui (`lib/Ui/`)

OLED-Display-Rendering ueber die U8g2-Bibliothek.

| Methode                                   | Beschreibung                        |
|-------------------------------------------|-------------------------------------|
| `Ui(int rstPin)`                          | Konstruktor mit Reset-Pin           |
| `begin()`                                 | Initialisiert Display               |
| `render(const char* timeStr, Logger& log)`| Zeichnet komplettes Display-Layout  |

**Display-Layout (128x64 Pixel):**

```
+----------------------------+
| Heltec Master              |   <- Titel (6x12 Font)
|                            |
|   14:32:07                 |   <- Uhrzeit (Logisoso18 Font)
|----------------------------|   <- Trennlinie (y=40)
| Log Zeile 1                |   <- Aeltester Eintrag
| Log Zeile 2                |
| Log Zeile 3                |   <- Neuester Eintrag
+----------------------------+
```

### BleSvc (`lib/BleSvc/`)

BLE-Peripheral mit Nordic UART Service (NUS). Empfaengt JSON-Befehle und sendet JSON-Antworten.

| Methode                 | Beschreibung                                |
|-------------------------|---------------------------------------------|
| `BleSvc::begin(Logger&)`| Initialisiert BLE, startet NUS + Advertising|

**Unterstuetzte Befehle:**

| Befehl (RX)                    | Antwort (TX)                                |
|-------------------------------|---------------------------------------------|
| `{"cmd":"get_info"}`          | `{"devicename":"...","serial":"...","battery":87,"position":"...","brightness":60,"temperature":23.4}` |
| Unbekannter Befehl            | `{"error":"unknown_cmd"}`                   |

**BLE-Details:**
- TX Power: `ESP_PWR_LVL_P9` (Maximum)
- Advertising Interval: 100ms (konfigurierbar)
- Security: Keine Authentifizierung (Open Pairing)
- Auto-Reconnect: Advertising startet automatisch nach Disconnect
- RX-Buffer: Max. 512 Bytes pro Zeile

---

## BLE-Architektur

### Rollen

| Komponente | BLE-Rolle  | GATT-Rolle  |
|-----------|------------|-------------|
| Heltec    | Peripheral | GATT Server |
| App       | Central    | GATT Client |

### Nordic UART Service (NUS) UUIDs

```
Service:  6E400001-B5A3-F393-E0A9-E50E24DCCA9E
RX Char:  6E400002-B5A3-F393-E0A9-E50E24DCCA9E  (Write / Write No Response)
TX Char:  6E400003-B5A3-F393-E0A9-E50E24DCCA9E  (Notify)
```

> **RX** = Daten von App an Device (Write)
> **TX** = Daten von Device an App (Notify)

### Advertising-Strategie

| Feld              | Inhalt              | Grund                                  |
|-------------------|---------------------|----------------------------------------|
| Primary Adv.      | NUS Service UUID    | Ermoeglicht gezielten Scan nach Service|
| Scan Response     | Device Name         | Android zeigt Namen zuverlaessig an    |

Die Trennung vermeidet Payload-Overflow im 31-Byte Primary Advertising.

---

## OLED-Display

- **Controller:** SSD1306
- **Aufloesung:** 128 x 64 Pixel
- **Interface:** I2C (Adresse `0x3C`)
- **Bibliothek:** U8g2 (Vollbild-Buffer-Modus)
- **Fonts:** `u8g2_font_6x12_tf` (Text), `u8g2_font_logisoso18_tf` (Uhrzeit)
- **Refresh:** ca. 20 FPS (50ms Loop-Delay)

---

## Konfiguration

Alle Einstellungen befinden sich in `include/config.h`:

### Feature Toggles

```cpp
#define WIFI_ENABLED  1    // 0 = WiFi/NTP deaktiviert
#define BLE_ENABLED   1    // 0 = BLE deaktiviert
```

### WiFi / NTP

```cpp
#define WIFI_SSID       "DEIN_SSID"
#define WIFI_PASSWORD   "DEIN_PASSWORT"
#define TZ_INFO         "CET-1CEST,M3.5.0/2,M10.5.0/3"  // Mitteleuropa
#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_SERVER_2    "time.nist.gov"
#define NTP_SERVER_3    "europe.pool.ntp.org"
```

### BLE

```cpp
#define BLE_DEVICE_NAME      "HeltecMaster"
#define BLE_ADV_INTERVAL_MS  100   // Advertising-Intervall (ms)
```

### Hardware Pins

```cpp
#define OLED_I2C_SDA    17
#define OLED_I2C_SCL    18
#define OLED_RST_PIN    21
#define VEXT_CTRL_PIN   36
```

> **Wichtig:** Vor dem Commit eigene WiFi-Zugangsdaten durch Platzhalter ersetzen!

---

## Entwicklungsumgebung einrichten

### Voraussetzungen

- Python 3.x
- Git

### 1. Repository klonen

```bash
git clone https://github.com/jwillner/heltec_master_lora.git
cd heltec_master_lora
```

### 2. Python Virtual Environment erstellen

**Windows (PowerShell):**

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements.txt
```

> Falls `Activate.ps1` blockiert wird:
> ```powershell
> Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
> ```

**Linux / macOS:**

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt
```

### 3. PlatformIO verifizieren

```bash
pio --version
```

---

## Build, Upload und Monitor

```bash
# Kompilieren
pio run

# Auf das Board flashen
pio run -t upload

# Serielle Ausgabe ueberwachen
pio device monitor -b 115200

# Alles in einem Schritt (Upload + Monitor)
pio run -t upload && pio device monitor -b 115200
```

---

## Projektstruktur

```
heltec_master_lora/
|
|-- include/
|   +-- config.h                 # Zentrale Konfiguration (Pins, WiFi, BLE, NTP)
|
|-- src/
|   +-- main.cpp                 # Einstiegspunkt (setup + loop)
|
|-- lib/
|   |-- Board/src/
|   |   |-- Board.h              # Hardware-Init Interface
|   |   +-- Board.cpp            # Vext, I2C, OLED Reset
|   |
|   |-- Logger/src/
|   |   |-- Logger.h             # Ringpuffer-Logger Interface
|   |   +-- Logger.cpp           # Serial + RAM Logging
|   |
|   |-- Log/src/
|   |   |-- Log.h                # Alternative LogRing Template-Klasse
|   |   +-- Log.cpp
|   |
|   |-- TimeSvc/src/
|   |   |-- TimeSvc.h            # NTP-Sync Interface
|   |   +-- TimeSvc.cpp          # WiFi connect, NTP sync, WiFi disconnect
|   |
|   |-- Ui/src/
|   |   |-- Ui.h                 # OLED Display Interface
|   |   +-- Ui.cpp               # U8g2 Rendering (Titel, Uhrzeit, Logs)
|   |
|   +-- BleSvc/src/
|       |-- BleSvc.h             # BLE NUS Interface
|       |-- BleSvc.cpp           # NimBLE Server, NUS, JSON Commands
|       +-- specification.md     # BLE-Spezifikation (Deutsch)
|
|-- platformio.ini               # PlatformIO Build-Konfiguration
|-- requirements.txt             # Python-Abhaengigkeiten (platformio)
|-- .gitignore                   # Git: ignoriert .pio, .venv, .vscode
+-- README.md                    # Diese Datei
```

---

## Ablauf (Setup / Loop)

### Setup-Phase

```
1. Logger initialisieren (Serial 115200 Baud)
2. Board::vextOn()          -> OLED-Stromversorgung aktivieren
3. Board::oledResetPulse()  -> OLED zuruecksetzen
4. Board::i2cBeginForOled() -> I2C-Bus starten
5. Ui::begin()              -> Display initialisieren
6. BleSvc::begin()          -> BLE + NUS starten (wenn BLE_ENABLED)
7. TimeSvc::syncOnce()      -> WiFi -> NTP -> WiFi aus (wenn WIFI_ENABLED)
```

### Loop-Phase (alle 50ms)

```
1. Aktuelle Uhrzeit formatieren (HH:MM:SS)
2. Alle 2 Sekunden: Tick-Nachricht loggen
3. Ui::render() -> Display aktualisieren (Titel + Uhrzeit + Logs)
4. delay(50ms)
```

---

## Geplante Erweiterungen

- LoRa-Kommunikation (SX1262, Modul `LoRaSvc` vorbereitet)
- Erweiterte BLE-Befehle (Steuerung, Konfiguration)
- Flutter-App als vollstaendiger BLE-Monitor und Controller
- Batterieueberwachung und Energiesparmodi
- OTA-Updates ueber BLE oder WiFi

---

## Abhaengigkeiten

| Bibliothek           | Version   | Zweck                    |
|---------------------|-----------|--------------------------|
| U8g2                | >= 2.36.17| OLED-Display (SSD1306)   |
| NimBLE-Arduino      | latest    | BLE Stack (h2zero Fork)  |
| PlatformIO Core     | >= 6.x    | Build-System             |

---

## Lizenz

Dieses Projekt ist derzeit ohne explizite Lizenz. Fuer Fragen zur Nutzung bitte den Autor kontaktieren.
