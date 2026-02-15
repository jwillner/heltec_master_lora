#pragma once

#include <stdint.h>

// ======================================================
// Project: Heltec WiFi LoRa 32 V3 – Master
// Step 1: OLED + Logger + NTP
// ======================================================

// ---------- Build Info ----------
#if defined(ROLE_SENDER)
  #define PROJECT_NAME      "Heltec Sender"
#elif defined(ROLE_RECEIVER)
  #define PROJECT_NAME      "Heltec Receiver"
#else
  #define PROJECT_NAME      "Heltec Master"
#endif
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
#ifndef WIFI_ENABLED
#define WIFI_ENABLED        1
#endif

#define WIFI_SSID           "YOUR_SSID"
#define WIFI_PASSWORD       "YOUR_PASSWORD"

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

#ifndef BLE_ENABLED
#define BLE_ENABLED           1
#endif

// Gerätename (so erscheint es beim Scannen)
#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME       "HeltecMaster"
#endif

// Advertising-Intervall in Millisekunden
// 100 ms = gut sichtbar, später evtl. 250–1000 ms für Strom sparen
#define BLE_ADV_INTERVAL_MS   100

// Nordic UART Service (NUS) UUIDs
#define NUS_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Write
#define NUS_TX_CHAR_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Notify

// ======================================================
// Geolocation (ip-api.com, IP-basiert, kein Key noetig)
// ======================================================
#ifndef GEO_ENABLED
#define GEO_ENABLED           1
#endif
#define GEO_API_URL           "http://ip-api.com/json/?fields=lat,lon"
#define GEO_TIMEOUT_MS        10000

// ======================================================
// Periodischer WiFi-Sync (NTP + Geo)
// ======================================================
#define WIFI_SYNC_INTERVAL_MS 300000   // 5 Minuten

// ======================================================
// LoRa (SX1262 onboard Heltec V3)
// ======================================================
#ifndef LORA_ENABLED
#define LORA_ENABLED          1
#endif
#define LORA_FREQUENCY        868.0    // MHz (EU)
#define LORA_BANDWIDTH        125.0    // kHz
#define LORA_SPREADING_FACTOR 7
#define LORA_CODING_RATE      5        // 4/5
#define LORA_SYNC_WORD        0x12
#define LORA_TX_POWER         14       // dBm
#define LORA_PREAMBLE_LEN     8
#define LORA_TCXO_VOLTAGE     1.6      // Heltec V3 TCXO

// Heltec V3 onboard SX1262 Pins
#define LORA_PIN_NSS          SS         // 8
#define LORA_PIN_DIO1         DIO0       // 14
#define LORA_PIN_RST          RST_LoRa   // 12
#define LORA_PIN_BUSY         BUSY_LoRa  // 13

#define LORA_SEND_INTERVAL_MS 30000    // 30s Sendeintervall
#define LORA_RX_GUARD_MS      5000     // ±5s Empfangsfenster
#define LORA_RX_WINDOW_MS     (LORA_RX_GUARD_MS * 2)  // 10s total
#define LORA_RX_MISS_EXTRA_MS 5000     // Extra-Wartezeit nach Timeout

// ======================================================
// Observer: Wio-SX1262 with XIAO ESP32S3 (B2B)
// ======================================================
#if defined(ROLE_OBSERVER)
  // Wio-SX1262 B2B LoRa Pins
  #undef  LORA_PIN_NSS
  #undef  LORA_PIN_DIO1
  #undef  LORA_PIN_RST
  #undef  LORA_PIN_BUSY
  #define LORA_PIN_NSS          41
  #define LORA_PIN_DIO1         39
  #define LORA_PIN_RST          42
  #define LORA_PIN_BUSY         40
  #define LORA_PIN_ANTSW        38       // Antenna switch (extern)

  // SPI pins (B2B wiring on XIAO ESP32S3)
  #define LORA_SPI_SCK          7
  #define LORA_SPI_MOSI         9
  #define LORA_SPI_MISO         8

  // UART Debug (XIAO ESP32S3)
  #define OBSERVER_UART_RX      44       // D6
  #define OBSERVER_UART_TX      43       // D7

  // Wio-SX1262 TCXO voltage differs from Heltec V3
  #undef  LORA_TCXO_VOLTAGE
  #define LORA_TCXO_VOLTAGE     1.8

  // Observer uses external antenna switch, not DIO2
  #define LORA_USE_DIO2_RFSWITCH 0
#else
  #define LORA_USE_DIO2_RFSWITCH 1
#endif

// ======================================================
// Device Configuration (BLE NUS set_config)
// ======================================================
enum DeviceMode : uint8_t {
  MODE_OFF = 0,
  MODE_STEADY,
  MODE_BLINK_ASYNC,
  MODE_BLINK_SYNC,
  MODE_BLINK_BACKLIGHT
};

struct DeviceConfig {
  char sn[32];
  uint8_t devId;
  DeviceMode mode;
};

extern DeviceConfig g_devCfg;

// Set to true once a valid set_config has been received via BLE.
// While false the device stays in configuration mode (BLE only, no sleep).
extern volatile bool g_configured;
