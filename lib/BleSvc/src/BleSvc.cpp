#include "BleSvc.h"
#include "config.h"
#if GEO_ENABLED
#include "GeoSvc.h"
#endif
#include "TimeSvc.h"

#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <string.h>

// ---- simple line assembler for RX ----
static String g_rxLineBuf;
static NimBLECharacteristic* g_txChar = nullptr;

// Utility: send one JSON line via TX notify
static void nus_send_json_line(const String& jsonLine) {
  if (!g_txChar) return;
  String out = jsonLine;
  if (!out.endsWith("\n")) out += "\n";
  g_txChar->setValue(out.c_str());
  g_txChar->notify();
}

// Mode string <-> enum helpers
static const char* const MODE_STRINGS[] = {
  "off", "steady", "blink_async", "blink_sync", "blink_backlight"
};
static const size_t MODE_COUNT = sizeof(MODE_STRINGS) / sizeof(MODE_STRINGS[0]);

static bool parseModeStr(const char* str, DeviceMode& out) {
  for (size_t i = 0; i < MODE_COUNT; i++) {
    if (strcmp(str, MODE_STRINGS[i]) == 0) {
      out = static_cast<DeviceMode>(i);
      return true;
    }
  }
  return false;
}

static const char* modeToStr(DeviceMode m) {
  if (m < MODE_COUNT) return MODE_STRINGS[m];
  return "off";
}

static String build_device_info_json() {
  JsonDocument doc;
  doc["devicename"] = BLE_DEVICE_NAME;
  doc["sn"] = g_devCfg.sn;
  doc["devId"] = g_devCfg.devId;
  doc["mode"] = modeToStr(g_devCfg.mode);
#if GEO_ENABLED
  doc["position"] = GeoSvc::valid() ? GeoSvc::position() : "0,0";
#else
  doc["position"] = "0,0";
#endif
  doc["temperature"] = 23.4f;
  doc["humidity"] = 55.0f;
  char timeBuf[16];
  TimeSvc::formatHHMMSS(timeBuf, sizeof(timeBuf));
  doc["time"] = timeBuf;

  String json;
  serializeJson(doc, json);
  return json;
}

static void handle_set_config(JsonDocument& doc) {
  // Parse optional fields
  if (doc["sn"].is<const char*>()) {
    strncpy(g_devCfg.sn, doc["sn"].as<const char*>(), sizeof(g_devCfg.sn) - 1);
    g_devCfg.sn[sizeof(g_devCfg.sn) - 1] = '\0';
  }

  if (doc["devId"].is<int>()) {
    g_devCfg.devId = doc["devId"].as<uint8_t>();
  }

  if (doc["mode"].is<const char*>()) {
    DeviceMode m;
    if (!parseModeStr(doc["mode"].as<const char*>(), m)) {
      nus_send_json_line("{\"error\":\"invalid mode\"}");
      return;
    }
    g_devCfg.mode = m;
  }

  // Mark device as configured → transitions from CONFIG to RUN mode
  g_configured = true;

  // Serial output
  Serial.printf("[BLE] set_config: sn=\"%s\" devId=%u mode=%s\n",
                g_devCfg.sn, g_devCfg.devId, modeToStr(g_devCfg.mode));

  // ACK response
  JsonDocument ack;
  ack["ack"] = true;
  ack["sn"] = g_devCfg.sn;
  ack["devId"] = g_devCfg.devId;
  ack["mode"] = modeToStr(g_devCfg.mode);
  String ackJson;
  serializeJson(ack, ackJson);
  nus_send_json_line(ackJson);
}

// ---- RX callback ----
class NusRxCallbacks : public NimBLECharacteristicCallbacks {
public:
  // Variant A (common)
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string v = pCharacteristic->getValue();
    if (v.empty()) return;
    handleBytes(v);
  }

  // Variant B (some NimBLE-Arduino versions)
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& /*connInfo*/) {
    std::string v = pCharacteristic->getValue();
    if (v.empty()) return;
    handleBytes(v);
  }

private:
  void handleBytes(const std::string& v) {
    for (char ch : v) {
      if (ch == '\r') continue;

      if (ch == '\n') {
        String line = g_rxLineBuf;
        g_rxLineBuf = "";

        line.trim();
        if (line.length() == 0) return;

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, line);
        if (err) {
          nus_send_json_line("{\"error\":\"invalid json\"}");
          return;
        }

        const char* cmd = doc["cmd"] | "";
        if (strcmp(cmd, "get_info") == 0) {
          nus_send_json_line(build_device_info_json());
        } else if (strcmp(cmd, "set_config") == 0) {
          handle_set_config(doc);
        } else {
          nus_send_json_line("{\"error\":\"unknown_cmd\"}");
        }
      } else {
        if (g_rxLineBuf.length() < 512) g_rxLineBuf += ch;
      }
    }
  }
};

// ---- server callbacks ----
class ServerCallbacks : public NimBLEServerCallbacks {
public:
  // Variant A
  void onConnect(NimBLEServer* /*pServer*/) {}

  void onDisconnect(NimBLEServer* /*pServer*/) {
    // Resume advertising so phone can reconnect
    NimBLEDevice::startAdvertising();
  }

  // Variant B (NimBLEConnInfo)
  void onConnect(NimBLEServer* /*pServer*/, NimBLEConnInfo& /*connInfo*/) {}

  void onDisconnect(NimBLEServer* /*pServer*/, NimBLEConnInfo& /*connInfo*/, int /*reason*/) {
    NimBLEDevice::startAdvertising();
  }
};

bool BleSvc::begin(Logger& log) {
#if !BLE_ENABLED
  (void)log;
  return true;
#else
  log.log("BLE: init...");

  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(false, false, false);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* nus = server->createService(NUS_SERVICE_UUID);

  NimBLECharacteristic* rx = nus->createCharacteristic(
    NUS_RX_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  rx->setCallbacks(new NusRxCallbacks());

  g_txChar = nus->createCharacteristic(
    NUS_TX_CHAR_UUID,
    NIMBLE_PROPERTY::NOTIFY
  );

  nus->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();

  // Keep primary advertising small: only NUS service UUID
  adv->addServiceUUID(NUS_SERVICE_UUID);

  // Put name into scan response (stable for Android)
  NimBLEAdvertisementData scanRsp;
  scanRsp.setName(BLE_DEVICE_NAME);
  adv->setScanResponseData(scanRsp);

  uint16_t interval = (uint16_t)(BLE_ADV_INTERVAL_MS / 0.625f);
  if (interval < 0x20) interval = 0x20;
  if (interval > 0x4000) interval = 0x4000;

  adv->setMinInterval(interval);
  adv->setMaxInterval(interval);

  adv->start();

  log.log(String("BLE: advertising '") + BLE_DEVICE_NAME + "'");
  return true;
#endif
}
