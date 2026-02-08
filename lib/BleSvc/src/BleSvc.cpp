#include "BleSvc.h"
#include "config.h"

#include <NimBLEDevice.h>

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

// Build device info JSON (dummy values for now)
static String build_device_info_json() {
  const String devName = BLE_DEVICE_NAME;
  const String serial = "HELTEC-001";
  int battery = 87;               // %
  float temperature = 23.4f;      // °C
  int brightness = 60;            // %
  String position = "47.12,8.55"; // dummy lat,lon

  String json = "{";
  json += "\"devicename\":\"" + devName + "\",";
  json += "\"serial\":\"" + serial + "\",";
  json += "\"battery\":" + String(battery) + ",";
  json += "\"position\":\"" + position + "\",";
  json += "\"brightness\":" + String(brightness) + ",";
  json += "\"temperature\":" + String(temperature, 1);
  json += "}";
  return json;
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

        if (line.indexOf("\"cmd\"") >= 0 && line.indexOf("get_info") >= 0) {
          nus_send_json_line(build_device_info_json());
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
