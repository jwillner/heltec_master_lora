# Heltec Master – BLE NUS Monitor
Specification Document (SPEC)

## 1. Ziel des Projekts

Ziel ist die Entwicklung eines plattformübergreifenden Systems bestehend aus:

- einem **Heltec WiFi LoRa 32 V3 (ESP32)** als BLE Device
- einer **Flutter Android App** als BLE Client

Die Kommunikation erfolgt über **Bluetooth Low Energy (BLE)** mittels  
**Nordic UART Service (NUS)**.

Die App dient als:
- BLE Scanner
- Device Connector
- Visualisierung von Geräteparametern
- Grundlage für spätere Steuer- und Monitoring-Funktionen

Der Fokus liegt auf:
- Stabiler BLE-Verbindung
- Klarer UI-Struktur
- JSON-basierter, textueller Kommunikation über NUS

---

## 2. Systemübersicht

### 2.1 Komponenten

#### Embedded Device
- Hardware: **Heltec WiFi LoRa 32 V3**
- MCU: ESP32
- BLE Stack: **NimBLE-Arduino (h2zero)**
- Rolle: BLE Peripheral / GATT Server

#### Mobile App
- Framework: **Flutter**
- Plattform: Android (minSdk 21)
- BLE Library: **flutter_blue_plus**
- Rolle: BLE Central / Client

---

## 3. BLE Architektur

### 3.1 BLE Rolle
| Komponente | Rolle |
|-----------|------|
| Heltec    | Peripheral |
| App       | Central |

### 3.2 Advertising (ESP32)

- Primary Advertising:
  - **NUS Service UUID**
- Scan Response:
  - **Device Name**

Grund:
- Android / Flutter zeigt Namen nur zuverlässig aus Scan Response
- Vermeidet Payload-Overflow im Primary Advertising

### 3.3 UUIDs (Nordic UART Service)

```text
Service UUID:  6E400001-B5A3-F393-E0A9-E50E24DCCA9E
RX Char UUID: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E (Write / Write No Response)
TX Char UUID: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E (Notify)
