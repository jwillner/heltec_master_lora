#pragma once
#include <Arduino.h>

namespace TimeSvc {
  bool valid();
  void formatHHMMSS(char* out, size_t len);

  bool wifiConnect(const char* ssid, const char* pass, void (*logFn)(const String&));
  void wifiDisconnect(void (*logFn)(const String&));
  void ntpSync(const char* tzInfo, void (*logFn)(const String&));

  // Convenience wrapper (connect → sync → disconnect)
  void syncOnce(const char* ssid,
                const char* pass,
                const char* tzInfo,
                void (*logFn)(const String&));
}
