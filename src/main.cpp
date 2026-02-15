#include <Arduino.h>
#include "config.h"

// ============================================================
// Observer: Minimal LoRa sniffer (Wio-SX1262 / XIAO ESP32S3)
// Nur Serial-Ausgabe, kein OLED/BLE/WiFi
// ============================================================
#if defined(ROLE_OBSERVER)

#include "LoRaSvc.h"

static uint32_t pktCount = 0;

static void logFn(const String& s) {
  Serial.println(s);
}

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  while (!Serial) delay(10);   // warten bis USB-CDC bereit
  delay(500);
  Serial.println();
  Serial.println("=== LoRa Observer (Wio-SX1262 / XIAO ESP32S3) ===");
  Serial.println("Freq: " + String(LORA_FREQUENCY, 1) + " MHz  "
                 "BW: " + String(LORA_BANDWIDTH, 0) + " kHz  "
                 "SF: " + String(LORA_SPREADING_FACTOR));

  LoRaSvc::begin(logFn);
  LoRaSvc::startReceive();
  Serial.println("Listening...");
}

void loop() {
  static uint32_t lastDot = 0;
  if (millis() - lastDot >= 1000) {
    lastDot = millis();
    Serial.print(".");
  }

  if (LoRaSvc::available()) {
    pktCount++;
    String msg  = LoRaSvc::readString();
    float rssi  = LoRaSvc::lastRssi();
    float snr   = LoRaSvc::lastSnr();

    Serial.println();  // Newline nach Punkt-Reihe
    unsigned long t = millis() / 1000;
    unsigned int h = t / 3600;
    unsigned int m = (t % 3600) / 60;
    unsigned int s = t % 60;

    Serial.printf("[%u] %02u:%02u:%02u RX <- \"%s\" | RSSI=%.1f dBm | SNR=%.1f dB | len=%u\n",
                  (unsigned)pktCount, h, m, s,
                  msg.c_str(), rssi, snr, (unsigned)msg.length());
  }
}

#else
// ============================================================
// Sender / Receiver: Heltec WiFi LoRa 32 V3 (vollständig)
// ============================================================

#include "Board.h"
#include "Logger.h"
#include "TimeSvc.h"
#include "Ui.h"
#include "BleSvc.h"
#if GEO_ENABLED
#include "GeoSvc.h"
#endif
#if LORA_ENABLED
#include "LoRaSvc.h"
#endif


DeviceConfig g_devCfg = {"", 0, MODE_OFF};
volatile bool g_configured = false;

BleSvc ble;

Logger logger(LOGGER_LINES);
Ui ui(OLED_RST_PIN);

static void logFn(const String& s) { logger.log(s); }

#if defined(ROLE_SENDER)
static uint32_t loraCounter = 0;
static uint32_t lastSend = 0;
#elif defined(ROLE_RECEIVER)
#include <esp_sleep.h>
static String lastRxMsg;
static float lastRssi = 0;
static float lastSnr  = 0;

enum RxState { INITIAL_SYNC, SLEEPING, LISTENING };
static RxState rxState = INITIAL_SYNC;
static uint32_t nextExpectedRx = 0;   // millis() wann nächstes Paket erwartet
static uint32_t windowStart    = 0;   // millis() wann aktuelles RX-Fenster begann
static uint32_t rxPktCount     = 0;
#endif

void setup() {
  logger.begin(SERIAL_BAUDRATE);

  Board::vextOn();
  Board::oledResetPulse();
  Board::i2cBeginForOled();

  ui.begin();
#if BLE_ENABLED
  ble.begin(logger);
#endif

  logger.log(PROJECT_NAME " v" PROJECT_VERSION);

#if WIFI_ENABLED
  if (TimeSvc::wifiConnect(WIFI_SSID, WIFI_PASSWORD, logFn)) {
    TimeSvc::ntpSync(TZ_INFO, logFn);
    #if GEO_ENABLED
    GeoSvc::fetchOnce(logFn);
    #endif
    TimeSvc::wifiDisconnect(logFn);
  }
#else
  logger.log("WLAN disabled");
#endif

  // LoRa is NOT started here – it will be initialised after BLE config
  logger.log("CONFIG MODE – waiting for BLE...");
}

void loop() {
  static bool loraStarted = false;

  // ---- CONFIG MODE: BLE only, no sleep, no LoRa ----
  if (!g_configured) {
    char timeBuf[16];
    TimeSvc::formatHHMMSS(timeBuf, sizeof(timeBuf));
    ui.render(timeBuf, "CONFIG MODE", 0, 0, "Waiting for BLE...", nullptr);
    delay(100);
    return;   // stay in config mode
  }

  // ---- One-time transition: start LoRa after first config ----
  if (!loraStarted) {
    loraStarted = true;
#if LORA_ENABLED
    LoRaSvc::begin(logFn);
  #if defined(ROLE_RECEIVER)
    LoRaSvc::startReceive();
    logger.log("[LoRa] Receiver mode");
  #elif defined(ROLE_SENDER)
    logger.log("[LoRa] Sender mode");
  #endif
#endif
    logger.log("Config received – running");
  }

  static uint32_t lastSync = 0;
#if defined(ROLE_RECEIVER)
  static uint32_t lastTick = 0;
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    Serial.print(".");
  }
#endif

  // Periodischer WiFi-Sync (NTP + Geo)
  #if WIFI_ENABLED
  if (millis() - lastSync > WIFI_SYNC_INTERVAL_MS) {
    lastSync = millis();
    if (TimeSvc::wifiConnect(WIFI_SSID, WIFI_PASSWORD, logFn)) {
      TimeSvc::ntpSync(TZ_INFO, logFn);
      #if GEO_ENABLED
      GeoSvc::fetchOnce(logFn);
      #endif
      TimeSvc::wifiDisconnect(logFn);
    }
  }
  #endif

  char timeBuf[16];
  TimeSvc::formatHHMMSS(timeBuf, sizeof(timeBuf));

#if GEO_ENABLED
  String pos = GeoSvc::valid() ? GeoSvc::position() : "no fix";
#else
  String pos = "--";
#endif

  float temperature = 23.4f;  // dummy
  float humidity    = 55.0f;  // dummy

  char loraBuf[40] = {0};

#if defined(ROLE_SENDER) && LORA_ENABLED
  if (millis() - lastSend >= LORA_SEND_INTERVAL_MS) {
    lastSend = millis();
    loraCounter++;
    String msg = String(loraCounter);
    if (LoRaSvc::sendString(msg)) {
      Serial.println("TX -> #" + msg);
    } else {
      Serial.println("TX FAIL #" + msg);
    }
  }
  snprintf(loraBuf, sizeof(loraBuf), "TX: #%lu  next %lds",
           (unsigned long)loraCounter,
           (long)((LORA_SEND_INTERVAL_MS - (millis() - lastSend)) / 1000));

#elif defined(ROLE_RECEIVER) && LORA_ENABLED
  switch (rxState) {

  case INITIAL_SYNC:
    snprintf(loraBuf, sizeof(loraBuf), "SYNC...");
    if (LoRaSvc::available()) {
      lastRxMsg = LoRaSvc::readString();
      lastRssi  = LoRaSvc::lastRssi();
      lastSnr   = LoRaSvc::lastSnr();
      rxPktCount++;
      nextExpectedRx = millis() + LORA_SEND_INTERVAL_MS;
      rxState = SLEEPING;
      Serial.print("\n");
      Serial.println("RX <- #" + lastRxMsg +
                     " RSSI=" + String(lastRssi, 1) +
                     " (initial sync)");
      snprintf(loraBuf, sizeof(loraBuf), "RX: #%s %.0fdBm",
               lastRxMsg.c_str(), lastRssi);
    }
    break;

  case SLEEPING: {
    uint32_t now = millis();
    int32_t wakeAt = (int32_t)(nextExpectedRx - LORA_RX_GUARD_MS);
    int32_t sleepMs = wakeAt - (int32_t)now;

    if (sleepMs > 100) {
      snprintf(loraBuf, sizeof(loraBuf), "SLEEP %lds", (long)(sleepMs / 1000));
      // Render display before sleep so user sees countdown
      {
        char preSleepTick[24] = {0};
        if (lastRxMsg.length() > 0) {
          snprintf(preSleepTick, sizeof(preSleepTick), "RX: #%s", lastRxMsg.c_str());
        }
        ui.render(timeBuf, pos.c_str(), temperature, humidity, loraBuf,
                  preSleepTick[0] ? preSleepTick : nullptr);
      }
      Serial.printf("\nSleeping %ld ms (wake at T-%ld)\n", (long)sleepMs, (long)LORA_RX_GUARD_MS);
      Serial.flush();
      esp_sleep_enable_timer_wakeup((uint64_t)sleepMs * 1000ULL);
      esp_light_sleep_start();
      // --- woke up ---
      Serial.println("Woke up, entering RX window");
    }
    // Transition to LISTENING
    LoRaSvc::startReceive();
    windowStart = millis();
    rxState = LISTENING;
    snprintf(loraBuf, sizeof(loraBuf), "LISTEN...");
    break;
  }

  case LISTENING: {
    uint32_t elapsed = millis() - windowStart;

    if (LoRaSvc::available()) {
      lastRxMsg = LoRaSvc::readString();
      lastRssi  = LoRaSvc::lastRssi();
      lastSnr   = LoRaSvc::lastSnr();
      rxPktCount++;
      // Re-sync: nächstes Paket in genau 30s ab jetzt
      nextExpectedRx = millis() + LORA_SEND_INTERVAL_MS;
      rxState = SLEEPING;
      Serial.print("\n");
      Serial.println("RX <- #" + lastRxMsg +
                     " RSSI=" + String(lastRssi, 1) +
                     " SNR=" + String(lastSnr, 1) +
                     " (pkt " + String(rxPktCount) + ")");
      snprintf(loraBuf, sizeof(loraBuf), "RX: #%s %.0fdBm",
               lastRxMsg.c_str(), lastRssi);
    } else if (elapsed >= LORA_RX_WINDOW_MS) {
      // Timeout — kein Paket empfangen
      Serial.println("\nRX window timeout, missed packet");
      nextExpectedRx += LORA_SEND_INTERVAL_MS;
      // Extra-Wartezeit bei nächstem Aufwachen (Fenster wird etwas früher geöffnet)
      nextExpectedRx -= LORA_RX_MISS_EXTRA_MS;
      rxState = SLEEPING;
      snprintf(loraBuf, sizeof(loraBuf), "MISS -> SLEEP");
    } else {
      uint32_t remaining = LORA_RX_WINDOW_MS - elapsed;
      snprintf(loraBuf, sizeof(loraBuf), "LISTEN %lds", (long)(remaining / 1000));
    }
    break;
  }

  } // switch
#endif

  const char* loraStr = (loraBuf[0] != '\0') ? loraBuf : nullptr;

  char tickBuf[24] = {0};
#if defined(ROLE_SENDER) && LORA_ENABLED
  snprintf(tickBuf, sizeof(tickBuf), "TX: #%lu", (unsigned long)loraCounter);
#elif defined(ROLE_RECEIVER) && LORA_ENABLED
  if (lastRxMsg.length() > 0) {
    snprintf(tickBuf, sizeof(tickBuf), "RX: #%s", lastRxMsg.c_str());
  }
#endif
  const char* tickStr = (tickBuf[0] != '\0') ? tickBuf : nullptr;

  ui.render(timeBuf, pos.c_str(), temperature, humidity, loraStr, tickStr);
  delay(50);
}

#endif // ROLE_OBSERVER
