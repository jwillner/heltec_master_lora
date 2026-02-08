#pragma once
#include <Arduino.h>

namespace TimeSvc {
  bool valid();
  void formatHHMMSS(char* out, size_t len);
  void syncOnce(const char* ssid,
                const char* pass,
                const char* tzInfo,
                void (*logFn)(const String&));
}
