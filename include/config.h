#pragma once

// ======================================================
// Project: Heltec WiFi LoRa 32 V3 – Master
// Step 1: OLED + Logger + NTP
// ======================================================

// ---------- Build Info ----------
#define PROJECT_NAME        "Heltec Master"
#define PROJECT_VERSION     "0.1.0"

// ---------- Serial ----------
#define SERIAL_BAUDRATE     115200

// ---------- Heltec WiFi LoRa 32 V3 Pins ----------
// OLED (I2C)
#define OLED_I2C_SDA        17
#define OLED_I2C_SCL        18
#define OLED_RST_PIN        21

// Vext (OLED power) – active LOW
#define VEXT_CTRL_PIN       36
#define VEXT_ON_LEVEL       LOW
#define VEXT_OFF_LEVEL      HIGH

// ---------- I2C ----------
#define I2C_FREQUENCY       100000

// ---------- OLED ----------
#define OLED_I2C_ADDRESS    0x3C
#define OLED_WIDTH          128
#define OLED_HEIGHT         64

// ---------- Logger ----------
#define LOGGER_LINES        3
#define LOGGER_TICK_MS      2000

// ---------- WLAN / NTP ----------
#define WIFI_ENABLED        1

#define WIFI_SSID           "xxxxx"
#define WIFI_PASSWORD       "xxxxx"

// CET / CEST
#define TZ_INFO             "CET-1CEST,M3.5.0/2,M10.5.0/3"

// NTP
#define NTP_SERVER_1        "pool.ntp.org"
#define NTP_SERVER_2        "time.nist.gov"
#define NTP_SERVER_3        "europe.pool.ntp.org"

// ---------- Time ----------
#define TIME_VALID_EPOCH    1700000000UL

// ======================================================
// BLE (Step 2.5: Advertising – Nordic UART Service)
// ======================================================

#define BLE_ENABLED           1

// Gerätename (so erscheint es beim Scannen)
#define BLE_DEVICE_NAME       "HeltecMaster"

// Advertising-Intervall in Millisekunden
// 100 ms = gut sichtbar, später evtl. 250–1000 ms für Strom sparen
#define BLE_ADV_INTERVAL_MS   100

// Nordic UART Service (NUS) UUIDs
#define NUS_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Write
#define NUS_TX_CHAR_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Notify

